#include <prtcl/rt/log/trace.hpp>

#if defined(PRTCL_RT_LOG_TRACE_TRACY)

#include <cstddef> // std::max_align_t
#include <new>

extern "C" {
#include <stdlib.h>
} //

#define TRACY_ENABLE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"

#include <TracyClient.cpp>

namespace prtcl::rt {

static void *
allocate_memory(size_t count, size_t alignment = alignof(std::max_align_t)) {
  return ::aligned_alloc(alignment, count);
}

static void free_memory(void *ptr) { ::free(ptr); }

} // namespace prtcl::rt

void *operator new(size_t count) {
  void *ptr = prtcl::rt::allocate_memory(count);
  TracyAlloc(ptr, count);
  return ptr;
}

void *operator new(size_t count, std::align_val_t alignment) {
  void *ptr = prtcl::rt::allocate_memory(count, static_cast<size_t>(alignment));
  TracyAlloc(ptr, count);
  return ptr;
}

void operator delete(void *ptr) noexcept {
  TracyFree(ptr);
  prtcl::rt::free_memory(ptr);
}

void operator delete(void *ptr, std::align_val_t) noexcept {
  TracyFree(ptr);
  prtcl::rt::free_memory(ptr);
}

#pragma GCC diagnostic pop

#endif // defined(PRTCL_RT_LOG_TRACE_TRACY)
