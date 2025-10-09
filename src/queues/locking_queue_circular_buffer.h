#include <cstddef>
#include <format>
#include <iostream>
#include <mutex>
#include <vector>

template <typename T> class locking_queue_with_circular_buffer {
private:
  std::size_t max_size_{};
  std::size_t size_{};

  std::size_t read_idx_{};
  std::size_t write_idx_{};

  std::vector<T> data_;
  std::mutex mutex_;

public:
  locking_queue_with_circular_buffer(size_t size = 100000)
      : data_(size, T{}), max_size_{size} {}

  bool try_put(const T &value) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ >= max_size_)
      return false;

    data_[write_idx_] = value;
    write_idx_ = (write_idx_ + 1) % max_size_;
    ++size_;
    return true;
  }

  std::optional<T> try_get() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (size_ <= 0)
      return std::nullopt;

    auto val = std::move(data_[read_idx_]);
    read_idx_ = (read_idx_ + 1) % max_size_;
    size_--;
    return val;
  }
};
