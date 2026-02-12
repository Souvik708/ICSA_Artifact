#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <array>
#include <atomic>
#include <string>

constexpr size_t QUEUE_SIZE = 30000;  // Adjust as needed

class LockFreeQueue {
 private:
  std::array<std::string, QUEUE_SIZE> buffer;
  std::atomic<size_t> head{0};
  std::atomic<size_t> tail{0};

  LockFreeQueue() = default;

 public:
  static LockFreeQueue& getInstance() {
    static LockFreeQueue instance;
    return instance;
  }

  bool push(const std::string& item) {
    size_t head_snapshot = head.load(std::memory_order_relaxed);
    size_t next_head = (head_snapshot + 1) % QUEUE_SIZE;

    // Check if the buffer is full
    if (next_head == tail.load(std::memory_order_acquire)) {
      return false;  // Queue full
    }

    buffer[head_snapshot] = item;  // Copy the string
    head.store(next_head, std::memory_order_release);
    return true;
  }

  bool pop(std::string& item) {
    size_t tail_snapshot = tail.load(std::memory_order_relaxed);

    // Check if the buffer is empty
    if (tail_snapshot == head.load(std::memory_order_acquire)) {
      return false;  // Queue empty
    }

    item = buffer[tail_snapshot];  // Move the string out
    tail.store((tail_snapshot + 1) % QUEUE_SIZE, std::memory_order_release);
    return true;
  }

  // Delete copy constructor and assignment operator to prevent copies
  LockFreeQueue(const LockFreeQueue&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&) = delete;
};

#endif  // LOCK_FREE_QUEUE_H