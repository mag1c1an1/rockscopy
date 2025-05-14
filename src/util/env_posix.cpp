
#include <string>

#include "leveldb/env.h"
#include "leveldb/status.h"
namespace leveldb {
namespace {

int g_open_read_only_file_limit = -1;
// Common flags defined for all posix open operations
#if defined(HAVE_O_CLOEXEC)
constexpr const int kOpenBaseFlags = O_CLOEXEC;
#else
constexpr const int kOpenBaseFlags = 0;
#endif  // defined(HAVE_O_CLOEXEC)

constexpr const size_t kWritableFileBufferSize = 65536;
}  // namespace

class PosixWritableFile final : public WritableFile {
 public:
  PosixWritableFile(const PosixWritableFile &) = delete;
  PosixWritableFile(PosixWritableFile &&) = delete;
  PosixWritableFile &operator=(const PosixWritableFile &) = delete;
  PosixWritableFile &operator=(PosixWritableFile &&) = delete;
  ~PosixWritableFile() override {}
  Status Append(const Slice &data) override {}

 private:
  // buf_[0, pos_ - 1] contains data to be written to fd_.
  char buf_[kWritableFileBufferSize];
  size_t pos_;
  int fd_;

  const bool is_manifest_;  // True if the file's name starts with MANIFEST.
  const std::string filename_;
  const std::string dirname_;  // The directory of filename_.
};

}  // namespace leveldb