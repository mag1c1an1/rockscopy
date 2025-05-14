// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Thread safety
// -------------
//
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the SkipList will not be destroyed
// while the read is in progress.  Apart from that, reads progress
// without any internal locking or synchronization.
//
// Invariants:
//
// (1) Allocated nodes are never deleted until the SkipList is
// destroyed.  This is trivially guaranteed by the code since we
// never delete any skip list nodes.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the SkipList.
// Only Insert() modifies the list, and it is careful to initialize
// a node and use release-stores to publish the nodes in one or
// more lists.
//
// ... prev vs. next pointer ordering ...
//

#pragma once

#include <cassert>
#include <cstdlib>

#include "port/atomic_pointer.h"
#include "util/arena.h"
#include "util/random.h"

namespace leveldb {

class Arena;

template <typename Key, class Comparator>
class SkipList {
 private:
  struct Node;

 public:
  explicit SkipList(Comparator cmp, Arena* arena);
  void Insert(const Key& key);
  bool Contains(const Key& key);

 private:
  enum { kMaxHeight = 12 };
  Comparator const compare_;
  Arena* const arena_;
  Node* const head_;
  port::AtomicPointer max_height_;
  // what is this
  Node* prev_[kMaxHeight];
  Random rnd_;

  inline int GetMaxHeight() const {
    return static_cast<int>(
        reinterpret_cast<intptr_t>(max_height_.NoBarrier_Load()));
  }

  Node* NewNode(const Key& key, int height);
  int RandomHeight();
  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }
  bool KeyIsAfterNode(const Key& key, Node* n) const;
  Node* FindGreatOrEqual(const Key& key, Node** prev) const;
  Node* FindLessThan(const Key& key) const;
  Node* FindLast() const;
};

template <typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node {
  explicit Node(const Key& k) : key(k) {}
  Key const key;
  Node* Next(int n) {
    assert(n >= 0);
    return reinterpret_cast<Node*>(next_[n].Acquire_Load());
  }
  void SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].Release_Store(x);
  }
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return reinterpret_cast<Node*>(next_[n].NoBarrier_Load());
  }
  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].NoBarrier_Store(x);
  }

 private:
  port::AtomicPointer next_[1];
};

template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena)
    : compare_(cmp),
      arena_(arena),
      head_(NewNode(0, kMaxHeight)),
      max_height_(reinterpret_cast<void*>(1)),
      rnd_(0xdeadbeef) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->SetNext(i, nullptr);
    prev_[i] = head_;
  }
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key) {
  Node* x = FindGreatOrEqual(key, prev_);
  int height = RandomHeight();
  if (height > GetMaxHeight()) {
    // 补充
    for (int i = GetMaxHeight(); i < height; i++) { prev_[i] = head_; }
    max_height_.NoBarrier_Store(reinterpret_cast<void*>(height));
  }
  x = NewNode(key, height);
  for (int i = 0; i < height; i++) {
    x->NoBarrier_Next(prev_[i]->NoBarrier_Next(i));
    prev_[i]->SetNext(i, x);
  }
  prev_[0] = x;
}
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) {
  Node* x = FindGreatOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

template <typename Key, class Comparator>
SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::SkipList::NewNode(
    const Key& key, int height) {
  char* mem = arena_->AllocateAligned(
      // Node contain one
      sizeof(Node) + sizeof(port ::AtomicPointer) * (height - 1));
  return new (mem) Node(key);
}

template <typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kMaxHeight && ((rnd_.Next() & kBranching) == 0)) { height++; }
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
  // n->key , key
  return (n != nullptr) && (compare_(n->key, key) < 0);
}
template <typename Key, class Comparator>
SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::SkipList::FindGreatOrEqual(const Key& key,
                                                      Node** prev) const {
  if (prev && !KeyIsAfterNode(key, prev[0]->Next(0))) {
    Node* x = prev[0];
    Node* next = x->Next(0);
    if ((x == head_) || KeyIsAfterNode(key, x)) { return next; }
  }
  // Normal
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {
      x = next;
    } else {
      // why this
      if (prev != nullptr) prev[level] = x;
      if (level == 0) {
        return next;
      } else {
        level--;
      }
    }
  }
}

template <typename Key, class Comparator>
SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLessThan(
    const Key& key) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->key, key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class Comparator>
SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast() const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        level--;
      }
    } else {
      x = next;
    }
  }
}

}  // namespace leveldb
