// SPDX-FileCopyrightText: LakeSoul Contributors
//
// SPDX-License-Identifier: Apache-2.0

#include "coding.h"

#include <algorithm>

namespace leveldb {

void EncodeFixed32(char *buf, uint32_t value) {
  memcpy(buf, &value, sizeof(value));
}

void EncodeFixed64(char *buf, uint64_t value) {
  memcpy(buf, &value, sizeof(value));
}

void PutFixed32(std::string *dst, uint32_t value) {
  char buf[sizeof(value)];
  EncodeFixed32(buf, value);
  dst->append(buf, sizeof(buf));
}

void PutFixed64(std::string *dst, uint64_t value) {
  char buf[sizeof(value)];
  EncodeFixed64(buf, value);
  dst->append(buf, sizeof(buf));
}

// 32位变长整数编码
char *EncodeVarint32(char *dst, uint32_t v) {
  // Operate on characters as unsigneds
  unsigned char *ptr = reinterpret_cast<unsigned char *>(dst);
  static constexpr int B = 128;
  if (v < (1 << 7)) {
    *(ptr++) = v;
  } else if (v < (1 << 14)) {
    *(ptr++) = v | B;
    *(ptr++) = v >> 7;
  } else if (v < (1 << 21)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = v >> 14;
  } else if (v < (1 << 28)) {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = v >> 21;
  } else {
    *(ptr++) = v | B;
    *(ptr++) = (v >> 7) | B;
    *(ptr++) = (v >> 14) | B;
    *(ptr++) = (v >> 21) | B;
    *(ptr++) = v >> 28;
  }
  return reinterpret_cast<char *>(ptr);
}

void PutVarint32(std::string *dst, uint32_t v) {
  char buf[5];
  char *ptr = EncodeVarint32(buf, v);
  dst->append(buf, ptr - buf);
}

char *EncodeVarint64(char *dst, uint64_t v) {
  static const unsigned int B = 128;
  unsigned char *ptr = reinterpret_cast<unsigned char *>(dst);
  while (v >= B) {
    *(ptr++) = (v & (B - 1)) | B;
    v >>= 7;
  }
  *(ptr++) = static_cast<unsigned char>(v);
  return reinterpret_cast<char *>(ptr);
}

void PutVarint64(std::string *dst, uint64_t v) {
  char buf[10];
  char *ptr = EncodeVarint64(buf, v);
  dst->append(buf, ptr - buf);
}

void PutLengthPrefixedSlice(std::string *dst, const Slice &value) {
  PutVarint32(dst, value.size());
  dst->append(value.data(), value.size());
}

int VarintLength(uint64_t v) {
  int len = 1;
  while (v >= 128) {
    v >>= 7;
    len++;
  }
  return len;
}

const char *GetVarint32PtrFallback(const char *p, const char *limit,
                                   uint32_t *value) {
  uint32_t result = 0;
  for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
    uint32_t byte = *(reinterpret_cast<const unsigned char *>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char *>(p);
    }
  }
  return nullptr;
}

bool GetVarint32(Slice *input, uint32_t *value) {
  const char *p = input->data();
  const char *limit = p + input->size();
  const char *q = GetVarint32Ptr(p, limit, value);
  if (q == nullptr) {
    return false;
  } else {
    *input = Slice(q, limit - q);
    return true;
  }
}

const char *GetVarint64Ptr(const char *p, const char *limit, uint64_t *value) {
  uint64_t result = 0;
  for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
    uint64_t byte = *(reinterpret_cast<const unsigned char *>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char *>(p);
    }
  }
  return nullptr;
}

bool GetVarint64(Slice *input, uint64_t *value) {
  const char *p = input->data();
  const char *limit = p + input->size();
  const char *q = GetVarint64Ptr(p, limit, value);
  if (q == nullptr) {
    return false;
  } else {
    *input = Slice(q, limit - q);
    return true;
  }
}

const char *GetLengthPrefixedSlice(const char *p, const char *limit,
                                   Slice *result) {
  uint32_t len;
  p = GetVarint32Ptr(p, limit, &len);
  if (p == nullptr) return nullptr;
  if (p + len > limit) return nullptr;
  *result = Slice(p, len);
  return p + len;
}

bool GetLengthPrefixedSlice(Slice *input, Slice *result) {
  uint32_t len;
  if (GetVarint32(input, &len) && input->size() >= len) {
    *result = Slice(input->data(), len);
    input->remove_prefix(len);
    return true;
  } else {
    return false;
  }
}

void BitStreamPutInt(char *dst, size_t dstlen, size_t offset, uint32_t bits,
                     uint64_t value) {
  assert((offset + bits + 7) / 8 <= dstlen);
  assert(bits <= 64);

  unsigned char *ptr = reinterpret_cast<unsigned char *>(dst);

  size_t byteOffset = offset / 8;
  size_t bitOffset = offset % 8;

  // This prevents unused variable warnings when compiling.
#ifndef NDEBUG
  // Store truncated value.
  uint64_t origValue =
      (bits < 64) ? (value & (((uint64_t) 1 << bits) - 1)) : value;
  uint32_t origBits = bits;
#endif

  while (bits > 0) {
    size_t bitsToGet = std::min<size_t>(bits, 8 - bitOffset);
    unsigned char mask = ((1 << bitsToGet) - 1);

    ptr[byteOffset] = (ptr[byteOffset] & ~(mask << bitOffset)) +
                      ((value & mask) << bitOffset);

    value >>= bitsToGet;
    byteOffset += 1;
    bitOffset = 0;
    bits -= bitsToGet;
  }

  assert(origValue == BitStreamGetInt(dst, dstlen, offset, origBits));
}

uint64_t BitStreamGetInt(const char *src, size_t srclen, size_t offset,
                         uint32_t bits) {
  assert((offset + bits + 7) / 8 <= srclen);
  assert(bits <= 64);

  const unsigned char *ptr = reinterpret_cast<const unsigned char *>(src);

  uint64_t result = 0;

  size_t byteOffset = offset / 8;
  size_t bitOffset = offset % 8;
  size_t shift = 0;

  while (bits > 0) {
    size_t bitsToGet = std::min<size_t>(bits, 8 - bitOffset);
    unsigned char mask = ((1 << bitsToGet) - 1);

    result += (uint64_t) ((ptr[byteOffset] >> bitOffset) & mask) << shift;

    shift += bitsToGet;
    byteOffset += 1;
    bitOffset = 0;
    bits -= bitsToGet;
  }

  return result;
}

void BitStreamPutInt(std::string *dst, size_t offset, uint32_t bits,
                     uint64_t value) {
  assert((offset + bits + 7) / 8 <= dst->size());

  const size_t kTmpBufLen = sizeof(value) + 1;
  char tmpBuf[kTmpBufLen];

  // Number of bytes of tmpBuf being used
  const size_t kUsedBytes = (offset % 8 + bits) / 8;

  // Copy relevant parts of dst to tmpBuf
  for (size_t idx = 0; idx <= kUsedBytes; ++idx) {
    tmpBuf[idx] = (*dst)[offset / 8 + idx];
  }

  BitStreamPutInt(tmpBuf, kTmpBufLen, offset % 8, bits, value);

  // Copy tmpBuf back to dst
  for (size_t idx = 0; idx <= kUsedBytes; ++idx) {
    (*dst)[offset / 8 + idx] = tmpBuf[idx];
  }

  // Do the check here too as we are working with a buffer.
  assert(((bits < 64) ? (value & (((uint64_t) 1 << bits) - 1)) : value) ==
         BitStreamGetInt(dst, offset, bits));
}

uint64_t BitStreamGetInt(const std::string *src, size_t offset, uint32_t bits) {
  return BitStreamGetInt(src->data(), src->size(), offset, bits);
}

uint64_t BitStreamGetInt(const Slice *src, size_t offset, uint32_t bits) {
  return BitStreamGetInt(src->data(), src->size(), offset, bits);
}

}  // namespace leveldb
