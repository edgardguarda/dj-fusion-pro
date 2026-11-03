// Fila sem trava, um produtor e um consumidor. O consumidor é a thread de áudio.
#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace djf {

template <typename T, size_t N>
class SpscQueue {
  static_assert(N >= 2 && (N & (N - 1)) == 0, "N deve ser potência de dois");

 public:
  // Capacidade útil: N - 1. Retorna false se estiver cheia.
  bool push(const T& v) {
    const size_t h = head_.load(std::memory_order_relaxed);
    const size_t next = (h + 1) & (N - 1);
    if (next == tail_.load(std::memory_order_acquire)) return false;
    buf_[h] = v;
    head_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& v) {
    const size_t t = tail_.load(std::memory_order_relaxed);
    if (t == head_.load(std::memory_order_acquire)) return false;
    v = buf_[t];
    tail_.store((t + 1) & (N - 1), std::memory_order_release);
    return true;
  }

 private:
  std::array<T, N> buf_{};
  alignas(64) std::atomic<size_t> head_{0};
  alignas(64) std::atomic<size_t> tail_{0};
};

}  // namespace djf
