#include "util/arena.h"

#include <cstddef>
#include <cstdint>

namespace leveldb {
constexpr int kBlockSize = 4096;

Arena::Arena()
    : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), blocks_memory_(0) {}

Arena::~Arena() {
  for (size_t i = 0; i < blocks_.size(); i++) { delete[] blocks_[i]; }
}

char *Arena::AllocateFallback(size_t bytes) {
  if (bytes > kBlockSize / 4) {
    // Object is more than a quarter of our block size.  Allocate it separately
    // to avoid wasting too much space in leftover bytes.
    char *result = AllocateNewBlock(bytes);
    return result;
  }

  // We waste the remaining space in the current block.
  alloc_ptr_ = AllocateNewBlock(kBlockSize);
  alloc_bytes_remaining_ = kBlockSize;

  char *result = alloc_ptr_;
  alloc_ptr_ += bytes;
  alloc_bytes_remaining_ -= bytes;
  return result;
}
char *Arena::AllocateNewBlock(size_t bytes) {
  char *result = new char[bytes];
  blocks_memory_ += bytes;
  blocks_.push_back(result);
  return result;
}

char *Arena::AllocateAligned(size_t bytes) {
  const int align = (sizeof(void *) > 8) ? sizeof(void *) : 8;
  static_assert((align & (align - 1)) == 0,
                "Pointer size should be a power of 2");
  size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
  size_t slop = (current_mod == 0 ? 0 : align - current_mod);
  size_t needed = bytes + slop;
  char *result;
  if (needed <= alloc_bytes_remaining_) {
    result = alloc_ptr_ + slop;
    alloc_ptr_ += needed;
    alloc_bytes_remaining_ -= needed;
  } else {
    // AllocateFallback always returned aligned memory
    result = AllocateFallback(bytes);
  }
  assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
  return result;
}

}  // namespace leveldb