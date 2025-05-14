// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// WriteBatch holds a collection of updates to apply atomically to a DB.
//
// The updates are applied in the order in which they are added
// to the WriteBatch.  For example, the value of "key" will be "v3"
// after the following batch is written:
//
//    batch.Put("key", "v1");
//    batch.Delete("key");
//    batch.Put("key", "v2");
//    batch.Put("key", "v3");
//
// Multiple threads can invoke const methods on a WriteBatch without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same WriteBatch must use
// external synchronization.

#pragma once

#include <string>

#include "leveldb/status.h"

namespace leveldb {

class Slice;

class WriteBatch {
public:
  WriteBatch();
  ~WriteBatch();

  // Store the mapping "key->value" in the database.
  void Put(const Slice &key, const Slice &value);

  // Merge "value" with the existing value of "key" in the database.
  // "key->merge(existing, value)"
  void Merge(const Slice &key, const Slice &value);

  // If the database contains a mapping for "key", erase it.  Else do nothing.
  void Delete(const Slice &key);

  // Clear all updates buffered in this batch.
  void Clear();

  // Support for iterating over the contents of a batch.
  class Handler {
  public:
    virtual ~Handler();
    virtual void Put(const Slice &key, const Slice &value) = 0;
    // Merge is not pure virtual. Otherwise, we would break existing
    // clients of Handler on a source code level.
    // The default implementation simply throws a runtime exception.
    virtual void Merge(const Slice &key, const Slice &value);
    virtual void Delete(const Slice &key) = 0;
  };
  Status Iterate(Handler *handler) const;

  // Retrive the serialized version of this batch.
  std::string Data() { return rep_; }

  // Constructor with a serialized string object
  WriteBatch(std::string rep) : rep_(rep) {}

private:
  friend class WriteBatchInternal;

  std::string rep_; // See comment in write_batch.cc for the format of rep_

  // Intentionally copyable
};

} // namespace leveldb
