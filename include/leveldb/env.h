// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An Env is an interface used by the leveldb implementation to access
// operating system functionality like the filesystem etc.  Callers
// may wish to provide a custom Env object when opening a database to
// get fine gain control; e.g., to rate limit file system operations.
//
// All Env implementations are safe for concurrent access from
// multiple threads without any external synchronization.

#pragma once

#include <cstdarg>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/status.h"

namespace leveldb {

class FileLock;
class Logger;
class RandomAccessFile;
class SequentialFile;
class Slice;
class WritableFile;
class Options;

using std::shared_ptr;
using std::unique_ptr;

// Options while opening a file to read/write
struct EnvOptions {

  // construct with default Options
  EnvOptions();

  // construct from Options
  EnvOptions(const Options &options);

  // If true, then allow caching of data in environment buffers
  bool use_os_buffer;

  // If true, then use mmap to read data
  bool use_mmap_reads;

  // If true, then use mmap to write data
  bool use_mmap_writes;

  // If true, set the FD_CLOEXEC on open fd.
  bool set_fd_cloexec;

  // Allows OS to incrementally sync files to disk while they are being
  // written, in the background. Issue one request for every bytes_per_sync
  // written. 0 turns it off.
  // Default: 0
  uint64_t bytes_per_sync;
};

class Env {
public:
  Env() {}
  virtual ~Env();

  // Return a default environment suitable for the current operating
  // system.  Sophisticated users may wish to provide their own Env
  // implementation instead of relying on this default environment.
  //
  // The result of Default() belongs to leveldb and must never be deleted.
  static Env *Default();

  // Create a brand new sequentially-readable file with the specified name.
  // On success, stores a pointer to the new file in *result and returns OK.
  // On failure stores nullptr in *result and returns non-OK.  If the file does
  // not exist, returns a non-OK status.
  //
  // The returned file will only be accessed by one thread at a time.
  virtual Status NewSequentialFile(const std::string &fname,
                                   unique_ptr<SequentialFile> *result,
                                   const EnvOptions &options) = 0;

  // Create a brand new random access read-only file with the
  // specified name.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.  If the file does not exist, returns a non-OK
  // status.
  //
  // The returned file may be concurrently accessed by multiple threads.
  virtual Status NewRandomAccessFile(const std::string &fname,
                                     unique_ptr<RandomAccessFile> *result,
                                     const EnvOptions &options) = 0;

  // Create an object that writes to a new file with the specified
  // name.  Deletes any existing file with the same name and creates a
  // new file.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.
  //
  // The returned file will only be accessed by one thread at a time.
  virtual Status NewWritableFile(const std::string &fname,
                                 unique_ptr<WritableFile> *result,
                                 const EnvOptions &options) = 0;

  // Returns true iff the named file exists.
  virtual bool FileExists(const std::string &fname) = 0;

  // Store in *result the names of the children of the specified directory.
  // The names are relative to "dir".
  // Original contents of *results are dropped.
  virtual Status GetChildren(const std::string &dir,
                             std::vector<std::string> *result) = 0;

  // Delete the named file.
  virtual Status DeleteFile(const std::string &fname) = 0;

  // Create the specified directory. Returns error if directory exists.
  virtual Status CreateDir(const std::string &dirname) = 0;

  // Creates directory if missing. Return Ok if it exists, or successful in
  // Creating.
  virtual Status CreateDirIfMissing(const std::string &dirname) = 0;

  // Delete the specified directory.
  virtual Status DeleteDir(const std::string &dirname) = 0;

  // Store the size of fname in *file_size.
  virtual Status GetFileSize(const std::string &fname, uint64_t *file_size) = 0;

  // Store the last modification time of fname in *file_mtime.
  virtual Status GetFileModificationTime(const std::string &fname,
                                         uint64_t *file_mtime) = 0;
  // Rename file src to target.
  virtual Status RenameFile(const std::string &src,
                            const std::string &target) = 0;

  // Lock the specified file.  Used to prevent concurrent access to
  // the same db by multiple processes.  On failure, stores nullptr in
  // *lock and returns non-OK.
  //
  // On success, stores a pointer to the object that represents the
  // acquired lock in *lock and returns OK.  The caller should call
  // UnlockFile(*lock) to release the lock.  If the process exits,
  // the lock will be automatically released.
  //
  // If somebody else already holds the lock, finishes immediately
  // with a failure.  I.e., this call does not wait for existing locks
  // to go away.
  //
  // May create the named file if it does not already exist.
  virtual Status LockFile(const std::string &fname, FileLock **lock) = 0;

  // Release the lock acquired by a previous successful call to LockFile.
  // REQUIRES: lock was returned by a successful LockFile() call
  // REQUIRES: lock has not already been unlocked.
  virtual Status UnlockFile(FileLock *lock) = 0;

  // Arrange to run "(*function)(arg)" once in a background thread.
  //
  // "function" may run in an unspecified thread.  Multiple functions
  // added to the same Env may run concurrently in different threads.
  // I.e., the caller may not assume that background work items are
  // serialized.
  virtual void Schedule(void (*function)(void *arg), void *arg) = 0;

  // Start a new thread, invoking "function(arg)" within the new thread.
  // When "function(arg)" returns, the thread will be destroyed.
  virtual void StartThread(void (*function)(void *arg), void *arg) = 0;

  // *path is set to a temporary directory that can be used for testing. It may
  // or many not have just been created. The directory may or may not differ
  // between runs of the same process, but subsequent calls will return the
  // same directory.
  virtual Status GetTestDirectory(std::string *path) = 0;

  // Create and return a log file for storing informational messages.
  virtual Status NewLogger(const std::string &fname,
                           shared_ptr<Logger> *result) = 0;

  // Returns the number of micro-seconds since some fixed point in time. Only
  // useful for computing deltas of time.
  virtual uint64_t NowMicros() = 0;

  // Sleep/delay the thread for the perscribed number of micro-seconds.
  virtual void SleepForMicroseconds(int micros) = 0;

  // Get the current host name.
  virtual Status GetHostName(char *name, uint64_t len) = 0;

  // Get the number of seconds since the Epoch, 1970-01-01 00:00:00 (UTC).
  virtual Status GetCurrentTime(int64_t *unix_time) = 0;

  // Get full directory name for this db.
  virtual Status GetAbsolutePath(const std::string &db_path,
                                 std::string *output_path) = 0;

  // The number of background worker threads for this environment.
  // default: 1
  virtual void SetBackgroundThreads(int number) = 0;

  // Converts seconds-since-Jan-01-1970 to a printable string
  virtual std::string TimeToString(uint64_t time) = 0;

private:
  // No copying allowed
  Env(const Env &);
  void operator=(const Env &);
};

// A file abstraction for reading sequentially through a file
class SequentialFile {
public:
  SequentialFile() {}
  virtual ~SequentialFile();

  // Read up to "n" bytes from the file.  "scratch[0..n-1]" may be
  // written by this routine.  Sets "*result" to the data that was
  // read (including if fewer than "n" bytes were successfully read).
  // May set "*result" to point at data in "scratch[0..n-1]", so
  // "scratch[0..n-1]" must be live when "*result" is used.
  // If an error was encountered, returns a non-OK status.
  //
  // REQUIRES: External synchronization
  virtual Status Read(size_t n, Slice *result, char *scratch) = 0;

  // Skip "n" bytes from the file. This is guaranteed to be no
  // slower that reading the same data, but may be faster.
  //
  // If end of file is reached, skipping will stop at the end of the
  // file, and Skip will return OK.
  //
  // REQUIRES: External synchronization
  virtual Status Skip(uint64_t n) = 0;
};

// A file abstraction for randomly reading the contents of a file.
class RandomAccessFile {
public:
  RandomAccessFile() {}
  virtual ~RandomAccessFile();

  // Read up to "n" bytes from the file starting at "offset".
  // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
  // to the data that was read (including if fewer than "n" bytes were
  // successfully read).  May set "*result" to point at data in
  // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
  // "*result" is used.  If an error was encountered, returns a non-OK
  // status.
  //
  // Safe for concurrent use by multiple threads.
  virtual Status Read(uint64_t offset, size_t n, Slice *result,
                      char *scratch) const = 0;

  // Tries to get an unique ID for this file that will be the same each time
  // the file is opened (and will stay the same while the file is open).
  // Furthermore, it tries to make this ID at most "max_size" bytes. If such an
  // ID can be created this function returns the length of the ID and places it
  // in "id"; otherwise, this function returns 0, in which case "id"
  // may not have been modified.
  //
  // This function guarantees, for IDs from a given environment, two unique ids
  // cannot be made equal to eachother by adding arbitrary bytes to one of
  // them. That is, no unique ID is the prefix of another.
  //
  // This function guarantees that the returned ID will not be interpretable as
  // a single varint.
  //
  // Note: these IDs are only valid for the duration of the process.
  virtual size_t GetUniqueId(char *id, size_t max_size) const {
    return 0; // Default implementation to prevent issues with backwards
              // compatibility.
  };

  enum AccessPattern { NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED };

  virtual void Hint(AccessPattern pattern) {}
};

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class WritableFile {
public:
  WritableFile() : last_preallocated_block_(0), preallocation_block_size_(0) {}
  virtual ~WritableFile();

  virtual Status Append(const Slice &data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0; // sync data

  /*
   * Sync data and/or metadata as well.
   * By default, sync only metadata.
   * Override this method for environments where we need to sync
   * metadata as well.
   */
  virtual Status Fsync() { return Sync(); }

  /*
   * Get the size of valid data in the file.
   */
  virtual uint64_t GetFileSize() { return 0; }

  /*
   * Get and set the default pre-allocation block size for writes to
   * this file.  If non-zero, then Allocate will be used to extend the
   * underlying storage of a file (generally via fallocate) if the Env
   * instance supports it.
   */
  void SetPreallocationBlockSize(size_t size) {
    preallocation_block_size_ = size;
  }

  virtual void GetPreallocationStatus(size_t *block_size,
                                      size_t *last_allocated_block) {
    *last_allocated_block = last_preallocated_block_;
    *block_size = preallocation_block_size_;
  }

protected:
  // PrepareWrite performs any necessary preparation for a write
  // before the write actually occurs.  This allows for pre-allocation
  // of space on devices where it can result in less file
  // fragmentation and/or less waste from over-zealous filesystem
  // pre-allocation.
  void PrepareWrite(size_t offset, size_t len) {
    if (preallocation_block_size_ == 0) {
      return;
    }
    // If this write would cross one or more preallocation blocks,
    // determine what the last preallocation block necesessary to
    // cover this write would be and Allocate to that point.
    const auto block_size = preallocation_block_size_;
    size_t new_last_preallocated_block =
        (offset + len + block_size - 1) / block_size;
    if (new_last_preallocated_block > last_preallocated_block_) {
      size_t num_spanned_blocks =
          new_last_preallocated_block - last_preallocated_block_;
      Allocate(block_size * last_preallocated_block_,
               block_size * num_spanned_blocks);
      last_preallocated_block_ = new_last_preallocated_block;
    }
  }

  /*
   * Pre-allocate space for a file.
   */
  virtual Status Allocate(off_t offset, off_t len) { return Status::OK(); }

  // Sync a file range with disk.
  // offset is the starting byte of the file range to be synchronized.
  // nbytes specifies the length of the range to be synchronized.
  // This asks the OS to initiate flushing the cached data to disk,
  // without waiting for completion.
  // Default implementation does nothing.
  virtual Status RangeSync(off_t offset, off_t nbytes) { return Status::OK(); }

private:
  size_t last_preallocated_block_;
  size_t preallocation_block_size_;
  // No copying allowed
  WritableFile(const WritableFile &);
  void operator=(const WritableFile &);
};

// An interface for writing log messages.
class Logger {
public:
  enum { DO_NOT_SUPPORT_GET_LOG_FILE_SIZE = -1 };
  Logger() {}
  virtual ~Logger();

  // Write an entry to the log file with the specified format.
  virtual void Logv(const char *format, va_list ap) = 0;
  virtual size_t GetLogFileSize() const {
    return DO_NOT_SUPPORT_GET_LOG_FILE_SIZE;
  }

private:
  // No copying allowed
  Logger(const Logger &);
  void operator=(const Logger &);
};

// Identifies a locked file.
class FileLock {
public:
  FileLock() {}
  virtual ~FileLock();

private:
  // No copying allowed
  FileLock(const FileLock &);
  void operator=(const FileLock &);
};

// Log the specified data to *info_log if info_log is non-nullptr.
extern void Log(const shared_ptr<Logger> &info_log, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__(__printf__, 2, 3)))
#endif
    ;

extern void Log(Logger *info_log, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__(__printf__, 2, 3)))
#endif
    ;

// A utility routine: write "data" to the named file.
extern Status WriteStringToFile(Env *env, const Slice &data,
                                const std::string &fname);

// A utility routine: read contents of named file into *data
extern Status ReadFileToString(Env *env, const std::string &fname,
                               std::string *data);

// An implementation of Env that forwards all calls to another Env.
// May be useful to clients who wish to override just part of the
// functionality of another Env.
class EnvWrapper : public Env {
public:
  // Initialize an EnvWrapper that delegates all calls to *t
  explicit EnvWrapper(Env *t) : target_(t) {}
  virtual ~EnvWrapper();

  // Return the target to which this Env forwards all calls
  Env *target() const { return target_; }

  // The following text is boilerplate that forwards all methods to target()
  Status NewSequentialFile(const std::string &f, unique_ptr<SequentialFile> *r,
                           const EnvOptions &options) {
    return target_->NewSequentialFile(f, r, options);
  }
  Status NewRandomAccessFile(const std::string &f,
                             unique_ptr<RandomAccessFile> *r,
                             const EnvOptions &options) {
    return target_->NewRandomAccessFile(f, r, options);
  }
  Status NewWritableFile(const std::string &f, unique_ptr<WritableFile> *r,
                         const EnvOptions &options) {
    return target_->NewWritableFile(f, r, options);
  }
  bool FileExists(const std::string &f) { return target_->FileExists(f); }
  Status GetChildren(const std::string &dir, std::vector<std::string> *r) {
    return target_->GetChildren(dir, r);
  }
  Status DeleteFile(const std::string &f) { return target_->DeleteFile(f); }
  Status CreateDir(const std::string &d) { return target_->CreateDir(d); }
  Status CreateDirIfMissing(const std::string &d) {
    return target_->CreateDirIfMissing(d);
  }
  Status DeleteDir(const std::string &d) { return target_->DeleteDir(d); }
  Status GetFileSize(const std::string &f, uint64_t *s) {
    return target_->GetFileSize(f, s);
  }

  Status GetFileModificationTime(const std::string &fname,
                                 uint64_t *file_mtime) {
    return target_->GetFileModificationTime(fname, file_mtime);
  }

  Status RenameFile(const std::string &s, const std::string &t) {
    return target_->RenameFile(s, t);
  }
  Status LockFile(const std::string &f, FileLock **l) {
    return target_->LockFile(f, l);
  }
  Status UnlockFile(FileLock *l) { return target_->UnlockFile(l); }
  void Schedule(void (*f)(void *), void *a) { return target_->Schedule(f, a); }
  void StartThread(void (*f)(void *), void *a) {
    return target_->StartThread(f, a);
  }
  virtual Status GetTestDirectory(std::string *path) {
    return target_->GetTestDirectory(path);
  }
  virtual Status NewLogger(const std::string &fname,
                           shared_ptr<Logger> *result) {
    return target_->NewLogger(fname, result);
  }
  uint64_t NowMicros() { return target_->NowMicros(); }
  void SleepForMicroseconds(int micros) {
    target_->SleepForMicroseconds(micros);
  }
  Status GetHostName(char *name, uint64_t len) {
    return target_->GetHostName(name, len);
  }
  Status GetCurrentTime(int64_t *unix_time) {
    return target_->GetCurrentTime(unix_time);
  }
  Status GetAbsolutePath(const std::string &db_path, std::string *output_path) {
    return target_->GetAbsolutePath(db_path, output_path);
  }
  void SetBackgroundThreads(int num) {
    return target_->SetBackgroundThreads(num);
  }
  std::string TimeToString(uint64_t time) {
    return target_->TimeToString(time);
  }

private:
  Env *target_;
};

} // namespace leveldb
