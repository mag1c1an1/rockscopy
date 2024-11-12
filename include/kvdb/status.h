#pragma once

namespace kvdb {
class Status {
private:
  enum class Code {
    Ok = 0,
    NotFound = 1,
    Corruption = 2,
    NotSupported = 3,
    InvalidArgument = 4,
    IOError = 5
  };

  Code code() const {
    return (state_ == nullptr) ? Code::Ok : static_cast<Code>(state_[4]);
  }
  // OK status has a null state_.  Otherwise, state_ is a new[] array
  // of the following form:
  //    state_[0..3] == length of message
  //    state_[4]    == code
  //    state_[5..]  == message
  const char *state_;
};
} // namespace kvdb