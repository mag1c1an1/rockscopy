#pragma once
namespace kvdb {
// A DB is a persistent ordered map from keys to values.
// A DB is safe for concurrent access from multiple threads without
// any external synchronization.
class DB {};
} // namespace kvdb