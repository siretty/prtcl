#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <variant>

#include <iostream>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace prtcl::rt {

template <typename T> class virtual_clock {
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

template <typename Clock_> class scheduler {
public:
  using clock_type = Clock_;
  using time_point = typename clock_type::time_point;
  using duration = typename clock_type::duration;

  struct do_nothing_type {};

  struct reschedule_after_type {
    duration after;
  };

  using callback_return_type =
      std::variant<do_nothing_type, reschedule_after_type>;

  callback_return_type do_nothing() const { return do_nothing_type{}; }

  template <typename Arg_>
  callback_return_type reschedule_after(Arg_ &&arg_) const {
    return reschedule_after_type{duration{std::forward<Arg_>(arg_)}};
  }

public:
  // callback(self, too_late)
  using callback_signature = callback_return_type(scheduler &, duration);

private:
  using callback_type = std::function<callback_signature>;

private:
  using schedule_map_type = boost::container::flat_multimap<
      time_point, callback_type, std::greater<time_point>>;

public:
  scheduler() : scheduler(std::make_shared<clock_type>()) {}

  scheduler(std::shared_ptr<clock_type> clock_) : _clock{clock_} {
    if (not clock_)
      throw "internal error: clock_ cannot be null";
  }

public:
  auto &clock() { return *_clock; }
  auto const &clock() const { return *_clock; }

public:
  template <typename Callback_>
  auto &schedule(time_point when_, Callback_ &&callback_) {
    using result_type =
        decltype(std::forward<Callback_>(callback_)(*this, duration{}));
    // depending on the result type of the callback ...
    if constexpr (std::is_same<result_type, void>::value)
      // ... schedule a callback that does not reschedule
      _schedule.emplace(
          when_, callback_type{[callback = std::forward<Callback_>(callback_)](
                                   auto &s, auto d) {
            callback(s, d);
            return s.do_nothing();
          }});
    else
      // ... simply schedule the callback
      _schedule.emplace(
          when_, callback_type{std::forward<Callback_>(callback_)});
    return *this;
  }

  template <typename Callback_>
  auto &schedule(duration after_, Callback_ &&callback_) {
    return schedule(_clock->now() + after_, std::forward<Callback_>(callback_));
  }

public:
  void tick() {
    auto now = _clock->now();
    auto first = _schedule.lower_bound(now);
    auto range = boost::make_iterator_range(first, _schedule.end());
    // call all scheduled callbacks
    for (auto &[when, callback] : range | boost::adaptors::reversed) {
      // invoke the callback
      auto result = callback(*this, now - when);
      // depending on the result, either ...
      if (auto *noop = std::get_if<do_nothing_type>(&result))
        // ... do nothing
        continue;
      else if (auto *ra = std::get_if<reschedule_after_type>(&result))
        // ... reschedule after the specified amount of time
        _next_schedule.emplace(now + ra->after, callback);
      else
        throw "internal error: callback return type not implemented";
    }
    // remove the callbacks we just invoked
    _schedule.erase(first, _schedule.end());
    // merge the next schedule
    _schedule.merge(_next_schedule);
    _next_schedule.clear();
  }

public:
  void clear() {
    _schedule.clear();
    _next_schedule.clear();
  }

private:
  std::shared_ptr<clock_type> _clock;

  schedule_map_type _schedule;
  schedule_map_type _next_schedule;
};

template <typename Rep_>
using virtual_scheduler = scheduler<virtual_clock<Rep_>>;

} // namespace prtcl::rt
