// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/status.h"

namespace leveldb {
class DBImpl : public DB {
public:
  DBImpl(const Options &options, const std::string &dbname);
  virtual ~DBImpl();
  // Implementations of the DB interface
  virtual Status Put(const WriteOptions &, const Slice &key,
                     const Slice &value);

  virtual Status Write(const WriteOptions &options, WriteBatch *updates);

private:
  struct DeletionState;
  void BackgroundCall();
  Status BackgroundCompaction(bool *madeProgress,
                              DeletionState &deletion_state);
};
} // namespace leveldb
