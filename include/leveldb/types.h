// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace leveldb {
//  Represents a sequence number in a WAL file.
using SequenceNumber = uint64_t;
} // namespace leveldb