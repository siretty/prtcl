#include "module_util.hpp"

#include <prtcl/util/hcp_lattice_source.hpp>
#include <prtcl/util/neighborhood.hpp>
#include <prtcl/util/scheduler.hpp>

#include <iomanip>
#include <sstream>

namespace prtcl::lua {

sol::table ModuleUtil(sol::state_view lua) {
  auto m = lua.create_table();

  {
    auto t = m.new_usertype<VirtualClock>("virtual_clock", sol::no_constructor);
    t["now"] = [](VirtualClock const &self) {
      return self.now().time_since_epoch().count();
    };

    t["reset"] = &VirtualClock::reset;

    t["set"] = [](VirtualClock &self, double when) {
      self.set(VirtualClock::time_point{VirtualClock::duration{when}});
    };

    t["advance"] = [](VirtualClock &self, double dur) {
      return self.advance(dur).time_since_epoch().count();
    };
  }

  {
    auto t = m.new_usertype<VirtualScheduler>(
        "virtual_scheduler", sol::constructors<VirtualScheduler()>());
    t["get_clock"] = &VirtualScheduler::GetClockPtr;

    t["tick"] = &VirtualScheduler::Tick;
    t["clear"] = &VirtualScheduler::Clear;

    t["do_nothing"] = &VirtualScheduler::DoNothing;
    t["reschedule_after"] = [](VirtualScheduler const &self, double after) {
      return self.RescheduleAfter(after);
    };

    t.set_function(
        "schedule_at",
        [](VirtualScheduler &self, double when,
           std::function<typename VirtualScheduler::CallbackSignature>
               callback) {
          VirtualScheduler::TimePoint when_tp{VirtualScheduler::Duration{when}};
          self.ScheduleAt(when_tp, callback);
        });

    t.set_function(
        "schedule_after",
        [](VirtualScheduler &self, double after,
           std::function<typename VirtualScheduler::CallbackSignature>
               callback) {
          VirtualScheduler::Duration after_dur{after};
          self.ScheduleAfter(after_dur, callback);
        });
  }

  {
    using VirtualDuration = typename VirtualScheduler::Duration;
    auto t = m.new_usertype<VirtualDuration>(
        "virtual_duration",
        sol::constructors<VirtualDuration(), VirtualDuration(double)>());

    t["seconds"] = sol::property(&VirtualDuration::count);

    t.set_function(
        sol::meta_function::to_string, [](VirtualDuration const &self) {
          std::ostringstream ss;
          ss << self.count() << "s";
          return ss.str();
        });
  }

  {
    using VirtualDuration = typename VirtualScheduler::Duration;
    using VirtualTimePoint = typename VirtualScheduler::TimePoint;
    auto t = m.new_usertype<VirtualTimePoint>(
        "virtual_time_point", "new",
        sol::factories(
            [] { return VirtualTimePoint{}; },
            [](double seconds) {
              return VirtualTimePoint{VirtualDuration{seconds}};
            },
            [](VirtualDuration duration) {
              return VirtualTimePoint{duration};
            }));

    t["since_epoch"] = sol::property(&VirtualTimePoint::time_since_epoch);

    t.set_function(
        sol::meta_function::to_string, [](VirtualTimePoint const &self) {
          std::ostringstream ss;

          auto total_seconds = self.time_since_epoch().count();
          auto total_minutes = static_cast<int>(std::floor(total_seconds / 60));
          auto hours = static_cast<int>(std::floor(total_minutes / 60));
          auto minutes =
              static_cast<int>(std::floor(total_minutes - hours * 60));
          auto seconds =
              static_cast<int>(std::floor(total_seconds - total_minutes * 60));

          ss << std::setfill('0') << std::setw(2) << hours;
          ss << ':' << std::setfill('0') << std::setw(2) << minutes;
          ss << ':' << std::setfill('0') << std::setw(2) << seconds;
          return ss.str();
        });
  }

  {
    auto t = m.new_usertype<Neighborhood>(
        "neighborhood", sol::constructors<Neighborhood()>());

    t["set_radius"] = &Neighborhood::SetRadius;
    t["load"] = &Neighborhood::Load;
    t["update"] = &Neighborhood::Update;
    t["permute"] = &Neighborhood::Permute;
  }

  {
    auto t = m.new_usertype<HCPLatticeSource>(
        "hcp_lattice_source",
        sol::constructors<HCPLatticeSource(
            Model &, Group &, double, DynamicTensorT<double, 1>,
            DynamicTensorT<double, 1>, cxx::count_t)>());

    t["regular_spawn_interval"] =
        sol::property(&HCPLatticeSource::GetRegularSpawnInterval);
  }

  return m;
}

} // namespace prtcl::lua
