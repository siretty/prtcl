#include <prtcl/rt/log/trace.hpp>

#if defined(PRTCL_RT_LOG_TRACE_TRACY)

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

void *operator new(size_t count) {
  void *ptr = malloc(count);
  TracyAlloc(ptr, count);
  return ptr;
}

void operator delete(void *ptr) noexcept {
  TracyFree(ptr);
  free(ptr);
}

#pragma GCC diagnostic pop

#endif // defined(PRTCL_RT_LOG_TRACE_TRACY)
