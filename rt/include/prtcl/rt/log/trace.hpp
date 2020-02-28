#pragma once

#include <sstream>
#include <string_view>
#include <utility>

#include <boost/preprocessor/variadic.hpp>

#if not defined(PRTCL_RT_LOG_TRACE_TRACY)

#define PRTCL_RT_LOG_TRACE_SCOPED(NAME_, ...)
#define PRTCL_RT_LOG_TRACE_CFRAME_MARK()
#define PRTCL_RT_LOG_TRACE_DFRAME_START(NAME_)
#define PRTCL_RT_LOG_TRACE_DFRAME_END(NAME_)

#endif // no tracing enabled

#if defined(PRTCL_RT_LOG_TRACE_TRACY)

#define TRACY_ENABLE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"

#include <Tracy.hpp>

#pragma GCC diagnostic pop

// enable memory profiling
void *operator new(size_t);
void operator delete(void *)noexcept;

#define PRTCL_RT_LOG_TRACE_SCOPED(...)                                         \
  _Pragma("GCC diagnostic push");                                              \
  _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"");                      \
  ZoneScopedN(BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__));                         \
  {                                                                            \
    std::ostringstream ss;                                                     \
    auto format = [&ss](auto &&... args) {                                     \
      (ss << ... << std::forward<decltype(args)>(args));                       \
      auto result = ss.str();                                                  \
      ss.str(std::string{});                                                   \
      return result;                                                           \
    };                                                                         \
    [&ss, &format](auto &&, auto &&... args) {                                 \
      (void)(format);                                                          \
      (ss << ... << std::forward<decltype(args)>(args));                       \
    }(__VA_ARGS__);                                                            \
    auto text = ss.str();                                                      \
    if (text.size() > 0)                                                       \
      ZoneText(text.data(), text.size());                                      \
  }                                                                            \
  _Pragma("GCC diagnostic pop");

#define PRTCL_RT_LOG_TRACE_CFRAME_MARK() FrameMark

#define PRTCL_RT_LOG_TRACE_DFRAME_START(NAME_) FrameMarkStart(NAME_)
#define PRTCL_RT_LOG_TRACE_DFRAME_END(NAME_) FrameMarkEnd(NAME_)

#define PRTCL_RT_LOG_TRACE_PLOT_NUMBER(NAME_, VALUE_)                          \
  _Pragma("GCC diagnostic push");                                              \
  _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"");                      \
  TracyPlot(NAME_, VALUE_);                                                    \
  TracyPlotConfig(NAME_, tracy::PlotFormatType::Number);                       \
  _Pragma("GCC diagnostic pop");

#endif // defined(PRTCL_RT_LOG_TRACE_TRACY)
