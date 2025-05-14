// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "leveldb/types.h"
#include "util/arena.h"
namespace leveldb {

class MemTable {
public:
  // No copying allowed
  MemTable(const MemTable &) = delete;
  void operator=(const MemTable &) = delete;

private:
  // Private since only Unref() should be used to delete it
  ~MemTable();
  int refs_;
  Arena arena_;
  // These are used to manage memtable flushes to storage
  bool flush_in_progress_; // started the flush
  bool flush_completed_;   // finished the flush
  uint64_t file_number_;   // filled up after flush is complete

  // The sequence number of the kv that was inserted first
  SequenceNumber first_seqno_;
  // The log files earlier than this number can be deleted.
  uint64_t mem_logfile_number_;
};

} // namespace leveldb