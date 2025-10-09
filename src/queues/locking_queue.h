#include <cstddef>
#include <mutex>
#include <queue>

template <typename T>
class locking_queue {
 private:
  std::queue<T> data_;
  std::mutex mutex_;

 public:
  locking_queue([[maybe_unused]] size_t size) {}

  bool try_put(const T& value) {
    std::unique_lock<std::mutex> lock(mutex_);

    data_.push(value);

    return true;
  }
  std::optional<T> try_get() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!data_.empty()) {
      auto val = data_.back();
      data_.pop();
      return {val};
    }

    return std::nullopt;
  }
};
