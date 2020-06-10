#ifndef PRTCL_VIRTUAL_CLOCK_HPP
#define PRTCL_VIRTUAL_CLOCK_HPP

#include <chrono>

namespace prtcl {

template <typename T>
class virtual_clock {
public:
  using rep = T;
  using period = std::ratio<1>;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<virtual_clock>;

public:
  constexpr static bool is_steady = false;

  time_point now() const { return now_; }

  void reset() { now_ = {}; }

  void set(time_point t) { now_ = t; }

  template <typename Rep, typename Period>
  time_point advance(std::chrono::duration<Rep, Period> const &d) {
    return now_ += std::chrono::duration_cast<duration>(d);
  }

  time_point advance(rep v_) { return advance(duration{v_}); }

private:
  time_point now_ = {};
};

} // namespace prtcl

#endif // PRTCL_VIRTUAL_CLOCK_HPP
