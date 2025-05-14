#include <gtest/gtest.h>

#include "util/arena.h"
#include "util/random.h"

namespace leveldb {
TEST(ArenaTest, Empty) {
  Arena arena;
}

TEST(ArenaTest, Simple) {
  std::vector<std::pair<size_t, char*>> allocated;
  Arena arena;
  const int N = 100000;  // 总共进行的分配次数
  size_t bytes = 0;      // 记录已分配的总字节数
  Random rnd(301);       // 初始化随机数生成器

  for (int i = 0; i < N; i++) {
    size_t s;
    // 根据循环次数 i 生成不同大小的分配请求
    if (i % (N / 10) == 0) {
      s = i;  // 每 10% 的时候，分配 i 字节
    } else {
      s = rnd.OneIn(4000)
              ? rnd.Uniform(6000)
              : (rnd.OneIn(10) ? rnd.Uniform(100) : rnd.Uniform(20));
    }

    // 确保分配大小不为 0
    if (s == 0) { s = 1; }

    char* r;
    // 选择对齐分配或普通分配
    if (rnd.OneIn(10)) {
      r = arena.AllocateAligned(s);
    } else {
      r = arena.Allocate(s);
    }

    // 用已知的位模式填充分配的内存
    for (unsigned int b = 0; b < s; b++) { r[b] = i % 256; }

    bytes += s;
    allocated.push_back(std::make_pair(s, r));  // 记录分配的内存和大小
    ASSERT_GE(arena.MemoryUsage(),
              bytes);  // 确保当前内存使用量大于等于总分配字节数
    if (i > N / 10) {
      ASSERT_LE(arena.MemoryUsage(),
                bytes * 1.10);  // 检查内存使用是否在合理范围内
    }
  }

  // 验证每个分配的内存块内容是否符合预期
  for (unsigned int i = 0; i < allocated.size(); i++) {
    size_t num_bytes = allocated[i].first;
    const char* p = allocated[i].second;
    for (unsigned int b = 0; b < num_bytes; b++) {
      ASSERT_EQ(int(p[b]) & 0xff, (int) (i % 256));  // 检查已知的位模式
    }
  }
}

}  // namespace leveldb