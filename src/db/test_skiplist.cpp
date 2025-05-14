#include <gtest/gtest.h>

#include <cstdint>

#include "skiplist.h"

// The purpose of these tests are just to  make sure that gtest was properly
// installed.

namespace leveldb {

using Key = uint64_t;

struct TestComparator {
  int operator()(const Key& a, const Key& b) const {
    if (a < b) {
      return -1;
    } else if (a > b) {
      return +1;
    } else {
      return 0;
    }
  }
};

TEST(SkipTest, Empty) {
  Arena arena;
  TestComparator cmp;
  SkipList<Key, TestComparator> list(cmp, &arena);
  ASSERT_TRUE(!list.Contains(10));

  SkipList<Key, TestComparator>::Iterator iter(&list);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToFirst();
  ASSERT_TRUE(!iter.Valid());
  iter.Seek(100);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToLast();
  ASSERT_TRUE(!iter.Valid());
}

}  // namespace leveldb
