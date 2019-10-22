#pragma once

#include <atomic>

namespace prtcl {

class spinlock {
public:
  spinlock() = default;

public:
  void lock() {
    while (_flag.test_and_set(std::memory_order_acquire))
      /* do nothing */;
  }

  void unlock() { _flag.clear(std::memory_order_release); }

private:
  std::atomic_flag _flag = ATOMIC_FLAG_INIT;
};

} // namespace prtcl
