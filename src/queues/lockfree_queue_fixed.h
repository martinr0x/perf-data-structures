#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>

// Lock-free fixed-size queue using sequence numbers for slots.
// Each slot has a sequence index to track its state.

template <typename T>
class lockfree_queue_fixed {
  struct Slot {
    std::atomic<std::size_t> sequence_idx_;
    T data_;
  };

private:
  std::size_t size_;
  std::atomic<std::size_t> read_idx_;
  std::atomic<std::size_t> write_idx_;
  std::unique_ptr<Slot[]> slots_;

public:
  lockfree_queue_fixed(size_t size = 100000)
      : size_{size}, read_idx_{0}, write_idx_{0}, slots_{std::make_unique<Slot[]>(size)} {
    for (size_t i = 0; i < size_; ++i) {
      slots_[i].sequence_idx_.store(i, std::memory_order_relaxed);
    }
  }

  template <typename U>
  bool try_put(U&& value) {
    size_t local_write_idx = write_idx_.load(std::memory_order_relaxed);
    while (true) {
      size_t local_read_idx = read_idx_.load(std::memory_order_acquire);
      if (local_write_idx - local_read_idx >= size_)
        return false;

      if (write_idx_.compare_exchange_weak(
              local_write_idx, local_write_idx + 1,
              std::memory_order_acq_rel, std::memory_order_relaxed)) {
        break;
      }
    }

    auto& slot = slots_[local_write_idx % size_];
    slot.data_ = std::forward<U>(value);
    slot.sequence_idx_.store(local_write_idx + 1, std::memory_order_release);
    return true;
  }

  std::optional<T> try_get() {
    size_t local_read_idx = read_idx_.load(std::memory_order_relaxed);
    while (true) {
      auto& slot = slots_[local_read_idx % size_];
      size_t local_sequence_idx = slot.sequence_idx_.load(std::memory_order_acquire);

      if (local_sequence_idx != local_read_idx + 1)
        return std::nullopt;

      T val = slot.data_;
      if (read_idx_.compare_exchange_weak(
              local_read_idx, local_read_idx + 1,
              std::memory_order_acq_rel, std::memory_order_relaxed)) {
        return std::optional<T>{std::move(val)};
      }
    }
  }
};
