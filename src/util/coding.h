// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

//
// Endian-neutral encoding:
// * Fixed-length numbers are encoded with least-significant byte first
// * In addition we support variable length "varint" encoding
// * Strings are encoded prefixed by their length in varint format

#pragma once
#include <cstdint>
#include <cstring>
#include <string>

#include "leveldb/slice.h"
#include "port/port.h"

namespace leveldb {

// The maximum length of a varint in bytes for 32 and 64 bits respectively.
const unsigned int kMaxVarint32Length = 5;
const unsigned int kMaxVarint64Length = 10;

// Standard Put... routines append to a string
void PutFixed32(std::string *dst, uint32_t value);
void PutFixed64(std::string *dst, uint64_t value);
void PutVarint32(std::string *dst, uint32_t value);
void PutVarint64(std::string *dst, uint64_t value);
void PutLengthPrefixedSlice(std::string *dst, const Slice &value);

// Standard Get... routines parse a value from the beginning of a Slice
// and advance the slice past the parsed value.
bool GetVarint32(Slice *input, uint32_t *value);
bool GetVarint64(Slice *input, uint64_t *value);
bool GetLengthPrefixedSlice(Slice *input, Slice *result);

// Pointer-based variants of GetVarint...  These either store a value
// in *v and return a pointer just past the parsed value, or return
// nullptr on error.  These routines only look at bytes in the range
// [p..limit-1]
const char *GetVarint32Ptr(const char *p, const char *limit, uint32_t *v);
const char *GetVarint64Ptr(const char *p, const char *limit, uint64_t *v);

// Returns the length of the varint32 or varint64 encoding of "v"
int VarintLength(uint64_t v);

// Lower-level versions of Put... that write directly into a character buffer
// REQUIRES: dst has enough space for the value being written
void EncodeFixed32(char *dst, uint32_t value);
void EncodeFixed64(char *dst, uint64_t value);

// Lower-level versions of Put... that write directly into a character buffer
// and return a pointer just past the last byte written.
// REQUIRES: dst has enough space for the value being written
char *EncodeVarint32(char *dst, uint32_t value);
char *EncodeVarint64(char *dst, uint64_t value);

// Lower-level versions of Get... that read directly from a character buffer
// without any bounds checking.

inline uint32_t DecodeFixed32(const char *ptr) {
  if (port::kLittleEndian) {
    // Load the raw bytes
    uint32_t result;
    memcpy(&result, ptr, sizeof(result)); // gcc optimizes this to a plain load
    return result;
  } else {
    return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0]))) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
            (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
  }
}

inline uint64_t DecodeFixed64(const char *ptr) {
  if (port::kLittleEndian) {
    // Load the raw bytes
    uint64_t result;
    memcpy(&result, ptr, sizeof(result)); // gcc optimizes this to a plain load
    return result;
  } else {
    uint64_t lo = DecodeFixed32(ptr);
    uint64_t hi = DecodeFixed32(ptr + 4);
    return (hi << 32) | lo;
  }
}

// Internal routine for use by fallback path of GetVarint32Ptr
const char *GetVarint32PtrFallback(const char *p, const char *limit,
                                   uint32_t *value);
inline const char *GetVarint32Ptr(const char *p, const char *limit,
                                  uint32_t *value) {
  if (p < limit) {
    uint32_t result = *(reinterpret_cast<const unsigned char *>(p));
    if ((result & 128) == 0) {
      *value = result;
      return p + 1;
    }
  }
  return GetVarint32PtrFallback(p, limit, value);
}

// Writes an unsigned integer with bits number of bits with its least
// significant bit at offset.
// Bits are numbered from 0 to 7 in the first byte, 8 to 15 in the second and
// so on.
// value is truncated to the bits number of least significant bits.
// REQUIRES: (offset+bits+7)/8 <= dstlen
// REQUIRES: bits <= 64
void BitStreamPutInt(char *dst, size_t dstlen, size_t offset, uint32_t bits,
                     uint64_t value);

// Reads an unsigned integer with bits number of bits with its least
// significant bit at offset.
// Bits are numbered in the same way as ByteStreamPutInt().
// REQUIRES: (offset+bits+7)/8 <= srclen
// REQUIRES: bits <= 64
uint64_t BitStreamGetInt(const char *src, size_t srclen, size_t offset,
                         uint32_t bits);

// Convenience functions
void BitStreamPutInt(std::string *dst, size_t offset, uint32_t bits,
                     uint64_t value);
uint64_t BitStreamGetInt(const std::string *src, size_t offset, uint32_t bits);
uint64_t BitStreamGetInt(const Slice *src, size_t offset, uint32_t bits);

} // namespace leveldb
