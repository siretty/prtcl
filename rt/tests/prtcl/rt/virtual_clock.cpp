#include <catch2/catch.hpp>

#include <memory>

#include <prtcl/rt/virtual_clock.hpp>

TEST_CASE("prtcl/rt/virtual_clock.hpp", "[prtcl][rt]") {
  SECTION("virtual clock") {
    prtcl::rt::virtual_clock<double> clock;
    auto initial_time = clock.now();

    CHECK(clock.now().time_since_epoch().count() == 0.0);

    clock.advance(1.0);
    CHECK(clock.now().time_since_epoch().count() == 1.0);

    clock.set(initial_time);
    CHECK(clock.now().time_since_epoch().count() == 0.0);

    clock.advance(2.0);
    CHECK(clock.now().time_since_epoch().count() == 2.0);

    clock.reset();
    CHECK(clock.now().time_since_epoch().count() == 0.0);
  }

  SECTION("virtual scheduler") {
    // create the shared clock
    auto clock = std::make_shared<prtcl::rt::virtual_clock<double>>();

    // create the scheduler using the shared clock
    prtcl::rt::virtual_scheduler<double> sched{clock};
    CHECK(clock->now() == sched.clock().now());

    // advance the shared clock
    clock->advance(1.0);
    CHECK(clock->now() == sched.clock().now());

    using duration = typename decltype(sched)::duration;
    int a = 0, b = 0;

    // schedule incrementing a initially after 2s, then reschedule every 1s
    sched.schedule(duration{2.0}, [&a](auto &s, auto) {
      ++a;
      return s.reschedule_after(1.0);
    });

    // schedule incrementing b once after 3s
    sched.schedule(duration{3.0}, [&b](auto &, auto) { ++b; });

    // check the initial states of a and b
    CHECK(a == 0);
    CHECK(b == 0);

    clock->advance(1.0);
    sched.tick();
    CHECK(a == 0);
    CHECK(b == 0);

    clock->advance(1.0);
    sched.tick();
    CHECK(a == 1);
    CHECK(b == 0);

    clock->advance(0.5);
    sched.tick();
    CHECK(a == 1);
    CHECK(b == 0);

    clock->advance(0.5);
    sched.tick();
    CHECK(a == 2);
    CHECK(b == 1);
  }
}
