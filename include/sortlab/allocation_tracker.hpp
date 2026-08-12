#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
#define SORTLAB_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define SORTLAB_NOINLINE __attribute__((noinline))
#else
#define SORTLAB_NOINLINE
#endif

namespace sortlab::allocation_tracker {

struct Snapshot {
  std::size_t calls = 0;
  std::size_t total_requested = 0;
  std::size_t peak_live = 0;
  std::size_t max_single = 0;
  std::size_t live_at_stop = 0;
};

struct alignas(std::max_align_t) Header {
  std::size_t size;
  bool tracked;
};

inline thread_local bool active = false;
inline thread_local Snapshot current{};
inline thread_local std::size_t live = 0;

inline void start() {
  current = {};
  live = 0;
  active = true;
}

inline Snapshot stop() {
  active = false;
  current.live_at_stop = live;
  return current;
}

inline void on_allocate(std::size_t bytes, bool tracked) {
  if (!tracked) return;
  ++current.calls;
  current.total_requested += bytes;
  current.max_single = std::max(current.max_single, bytes);
  live += bytes;
  current.peak_live = std::max(current.peak_live, live);
}

inline void on_deallocate(const Header& header) {
  if (!header.tracked) return;
  live = header.size <= live ? live - header.size : 0;
}

}  // namespace sortlab::allocation_tracker

SORTLAB_NOINLINE inline void* operator new(std::size_t bytes) {
  using namespace sortlab::allocation_tracker;
  const std::size_t allocation_bytes = sizeof(Header) + (bytes == 0 ? 1 : bytes);
  void* raw = std::malloc(allocation_bytes);
  if (raw == nullptr) throw std::bad_alloc();
  auto* header = static_cast<Header*>(raw);
  header->size = bytes;
  header->tracked = active;
  on_allocate(bytes, header->tracked);
  return static_cast<void*>(header + 1);
}

SORTLAB_NOINLINE inline void* operator new[](std::size_t bytes) {
  return ::operator new(bytes);
}

SORTLAB_NOINLINE inline void operator delete(void* pointer) noexcept {
  if (pointer == nullptr) return;
  using namespace sortlab::allocation_tracker;
  auto* header = static_cast<Header*>(pointer) - 1;
  on_deallocate(*header);
  std::free(header);
}

SORTLAB_NOINLINE inline void operator delete[](void* pointer) noexcept {
  ::operator delete(pointer);
}

SORTLAB_NOINLINE inline void operator delete(void* pointer, std::size_t) noexcept {
  ::operator delete(pointer);
}

SORTLAB_NOINLINE inline void operator delete[](void* pointer, std::size_t) noexcept {
  ::operator delete(pointer);
}

#undef SORTLAB_NOINLINE
