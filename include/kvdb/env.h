#pragma once

#include <cstdarg>

#include "kvdb/slice.h"
#include "kvdb/status.h"

namespace kvdb {

class WritableFile {
public:
  WritableFile() = default;
  WritableFile(const WritableFile &) = delete;
  WritableFile &operator=(const WritableFile &) = delete;

  virtual ~WritableFile();

  virtual Status Append(const Slice &data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;
};

class Logger {
  virtual ~Logger();
  // Write an entry to the log file with the specified format.
  virtual void Logv(const char *format, std::va_list ap) = 0;
};
}; // namespace kvdb

// namespace kvdb