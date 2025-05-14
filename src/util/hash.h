// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>

namespace leveldb {
uint32_t Hash(const char *data, size_t n, uint32_t seed);
}