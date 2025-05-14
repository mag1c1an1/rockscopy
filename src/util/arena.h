#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

namespace leveldb {

// const method do not need external sync

class Arena {
 public:
  Arena();

  // no coping
  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  ~Arena();

  // Return a pointer to a newly allocated memory block of "bytes" bytes.
  char *Allocate(size_t bytes);

  // Allocate memory with the normal alignment guarantees provided by malloc.
  char *AllocateAligned(size_t bytes);

  // Returns an estimate of the total memory usage of data allocated
  // by the arena.
  size_t MemoryUsage() const {
    return blocks_memory_ + blocks_.capacity() * sizeof(char *);
  }

 private:
  char *AllocateFallback(size_t bytes);
  char *AllocateNewBlock(size_t block_bytes);

  // Allocation state
  char *alloc_ptr_;
  size_t alloc_bytes_remaining_;

  // Array of new[] allocated memory blocks
  std::vector<char *> blocks_;

  // Bytes of memory in blocks allocated so far
  size_t blocks_memory_;
};

inline char *Arena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    char *result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
  }
  return AllocateFallback(bytes);
}

}  // namespace leveldb
