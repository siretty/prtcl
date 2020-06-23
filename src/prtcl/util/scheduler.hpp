#ifndef PRTCL_SCHEDULER_HPP
#define PRTCL_SCHEDULER_HPP

#include "virtual_clock.hpp"

#include <memory>
#include <variant>

#include <boost/container/flat_map.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace prtcl {

template <typename Clock>
class Scheduler {
public:
  using ClockType = Clock;
  using TimePoint = typename ClockType::time_point;
  using Duration = typename ClockType::duration;

  struct DoNothingType {};

  struct RescheduleAfterType {
    Duration after;
  };

  struct RescheduleAtType {
    TimePoint when;
  };

  using CallbackReturnType =
      std::variant<DoNothingType, RescheduleAfterType, RescheduleAtType>;

  CallbackReturnType DoNothing() const { return DoNothingType{}; }

  template <typename Arg_>
  CallbackReturnType RescheduleAfter(Arg_ &&arg_) const {
    return RescheduleAfterType{Duration{std::forward<Arg_>(arg_)}};
  }

  template <typename Arg_>
  CallbackReturnType RescheduleAt(Arg_ &&arg_) const {
    return RescheduleAtType{TimePoint{std::forward<Arg_>(arg_)}};
  }

public:
  // callback(self, too_late)
  using CallbackSignature = CallbackReturnType(Scheduler &, Duration);

private:
  using CallbackType = std::function<CallbackSignature>;

private:
  using ScheduleMapType = boost::container::flat_multimap<
      TimePoint, CallbackType, std::greater<TimePoint>>;

public:
  Scheduler() : Scheduler(std::make_shared<ClockType>()) {}

  Scheduler(std::shared_ptr<ClockType> clock) : clock_{clock} {
    if (not clock)
      throw "internal error: GetClock cannot be null";
  }

public:
  auto const &GetClock() const { return *clock_; }

  std::shared_ptr<ClockType> GetClockPtr() { return clock_; }

public:
  template <typename Callback_>
  auto &ScheduleAt(TimePoint after_, Callback_ &&callback_) {
    using result_type =
        decltype(std::forward<Callback_>(callback_)(*this, Duration{}));
    // depending on the result type of the callback ...
    if constexpr (std::is_same<result_type, void>::value)
      // ... schedule a callback that does not reschedule
      schedule_.emplace(
          after_, CallbackType{[callback = std::forward<Callback_>(callback_)](
                                   auto &s, auto d) {
            callback(s, d);
            return s.do_nothing();
          }});
    else
      // ... simply schedule the callback
      schedule_.emplace(
          after_, CallbackType{std::forward<Callback_>(callback_)});
    return *this;
  }

  template <typename Callback_>
  auto &ScheduleAfter(Duration after_, Callback_ &&callback_) {
    return ScheduleAt(
        clock_->now() + after_, std::forward<Callback_>(callback_));
  }

public:
  void Tick() {
    auto now = clock_->now();
    auto first = schedule_.lower_bound(now);
    auto range = boost::make_iterator_range(first, schedule_.end());
    // call all scheduled callbacks
    for (auto &[when, callback] : range | boost::adaptors::reversed) {
      // invoke the callback
      auto result = callback(*this, now - when);
      // depending on the result, either ...
      if (auto *noop = std::get_if<DoNothingType>(&result))
        // ... do nothing
        continue;
      else if (auto *raf = std::get_if<RescheduleAfterType>(&result))
        // ... reschedule after the specified amount of time
        next_schedule_.emplace(now + raf->after, callback);
      else if (auto *rat = std::get_if<RescheduleAtType>(&result))
        // ... reschedule at the specified time point
        next_schedule_.emplace(rat->when, callback);
      else
        throw "internal error: callback return type not implemented";
    }
    // remove the callbacks we just invoked
    schedule_.erase(first, schedule_.end());
    // merge the next ScheduleAt
    schedule_.merge(next_schedule_);
    next_schedule_.clear();
  }

public:
  void Clear() {
    schedule_.clear();
    next_schedule_.clear();
  }

private:
  std::shared_ptr<ClockType> clock_;

  ScheduleMapType schedule_;
  ScheduleMapType next_schedule_;
};

using VirtualScheduler = Scheduler<VirtualClock>;

} // namespace prtcl

#endif // PRTCL_SCHEDULER_HPP
