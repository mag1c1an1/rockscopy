// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>

namespace leveldb::port {
class AtomicPointer {
 public:
  AtomicPointer() {}
  explicit AtomicPointer(void *v) : rep_(v) {}
  inline void *Acquire_Load() const {
    return rep_.load(std::memory_order_acquire);
  }
  inline void Release_Store(void *v) {
    rep_.store(v, std::memory_order_release);
  }
  inline void *NoBarrier_Load() const {
    return rep_.load(std::memory_order_relaxed);
  }
  inline void NoBarrier_Store(void *v) {
    rep_.store(v, std::memory_order_relaxed);
  }

 private:
  std::atomic<void *> rep_;
};
}  // namespace leveldb::port