// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <pthread.h>

#include "atomic_pointer.h"

namespace leveldb::port {
static constexpr bool kLittleEndian = true;

class CondVar;
class Mutex {
 public:
  Mutex(bool adaptive = false);
  ~Mutex();
  void Lock();
  void Unlock();
  void AssertHeld() {}

  // No copying
  Mutex(const Mutex &) = delete;
  void operator=(const Mutex &) = delete;

 private:
  friend class CondVar;
  pthread_mutex_t mu_;
};

}  // namespace leveldb::port