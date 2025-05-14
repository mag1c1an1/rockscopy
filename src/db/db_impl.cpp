// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#include "db/db_impl.h"

#include "leveldb/status.h"

namespace leveldb {

// Default implementations of convenience methods that subclasses of DB
// can call if they wish
Status DB::Put(const WriteOptions &opt, const Slice &key, const Slice &value) {
  WriteBatch batch;
  batch.Put(key, value);
  return Write(opt, &batch);
}

Status DBImpl::Write(const WriteOptions &options, WriteBatch *my_batch) {}

}  // namespace leveldb