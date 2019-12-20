#include <catch.hpp>

#include <chrono>
#include <iostream>

#include <prtcl/data/host/grouped_uniform_grid.hpp>
#include <prtcl/openmp_neighbourhood.hpp>
#include <prtcl/scheme/test.hpp>

TEST_CASE("prtcl/scheme/test", "[prtcl]") {
  namespace tag = ::prtcl::tag;

  ::prtcl::data::scheme<float, 3> scheme;
  {
    using namespace ::prtcl::expr_literals;
    auto &o1 = scheme.add_group("o1").set_type("other");
    o1.resize(10'000);
    o1.add("position"_vvf);

    auto &o2 = scheme.add_group("o2").set_type("something_else");
    o2.resize(10'000);
    o2.add("position"_vvf);

    auto &f1 = scheme.add_group("f1").set_type("fluid");
    f1.resize(10'000);
    f1.add("position"_vvf);

    auto &f2 = scheme.add_group("f2").set_type("fluid");
    f2.resize(10'000);
    f2.add("position"_vvf);

    auto &b1 = scheme.add_group("b1").set_type("boundary");
    b1.resize(10'000);
    b1.add("position"_vvf);

    auto &b2 = scheme.add_group("b2").set_type("boundary");
    b2.resize(10'000);
    b2.add("position"_vvf);
  }

  ::prtcl::scheme::TEST<float, 3> test;
  test.require(scheme);

  ::prtcl::openmp_neighbourhood<::prtcl::grouped_uniform_grid<float, 3>>
      neighbourhood;
  neighbourhood.load(scheme);

  std::cerr << "neighbourhood.update() ..." << '\n';
  neighbourhood.update();
  std::cerr << "... done" << '\n';

  test.load(scheme);

  {
    std::cerr << "test.Test(neighbourhood) ..." << '\n';
    auto beg = std::chrono::high_resolution_clock::now();
    test.Test(neighbourhood);
    auto dur = std::chrono::high_resolution_clock::now() - beg;
    std::cerr << "... done "
              << static_cast<std::chrono::duration<float>>(dur).count() << "s"
              << '\n';
  }

  for (auto name : scheme.get(tag::type::scalar{}).names())
    std::cerr << "gs " << name << '\n';
  for (auto name : scheme.get(tag::type::vector{}).names())
    std::cerr << "gv " << name << '\n';
  for (auto name : scheme.get(tag::type::matrix{}).names())
    std::cerr << "gm " << name << '\n';

  for (auto [i, n, group] : scheme.enumerate_groups()) {
    std::cerr << "group #" << i << " " << n << " type=" << group.get_type()
              << '\n';

    for (auto name :
         group.get(tag::kind::uniform{}, tag::type::scalar{}).names())
      std::cerr << "  us " << name << '\n';
    for (auto name :
         group.get(tag::kind::uniform{}, tag::type::vector{}).names())
      std::cerr << "  uv " << name << '\n';
    for (auto name :
         group.get(tag::kind::uniform{}, tag::type::matrix{}).names())
      std::cerr << "  um " << name << '\n';

    for (auto name :
         group.get(tag::kind::varying{}, tag::type::scalar{}).names())
      std::cerr << "  vs " << name << '\n';
    for (auto name :
         group.get(tag::kind::varying{}, tag::type::vector{}).names())
      std::cerr << "  vv " << name << '\n';
    for (auto name :
         group.get(tag::kind::varying{}, tag::type::matrix{}).names())
      std::cerr << "  vm " << name << '\n';
  }
}
