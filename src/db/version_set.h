// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace leveldb {

class Version {
public:
  // No copying allowed
  Version(const Version &) = delete;
  void operator=(const Version &) = delete;

private:
};

class VersionSet {};

// A Compaction encapsulates information about a compaction.
class Compaction {};

} // namespace leveldb