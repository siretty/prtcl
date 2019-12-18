#include <catch.hpp>

#include <iostream>

#include <prtcl/scheme/test.hpp>

TEST_CASE("prtcl/scheme/test", "[prtcl]") {
  namespace tag = ::prtcl::tag;

  ::prtcl::data::scheme<float, 3> scheme;
  scheme.add_group("o1").set_type("other");
  scheme.add_group("o2").set_type("something_else");
  scheme.add_group("f1").set_type("fluid").resize(2048);
  scheme.add_group("f2").set_type("fluid").resize(4096);
  scheme.add_group("b1").set_type("boundary").resize(512);
  scheme.add_group("b2").set_type("boundary").resize(1024);

  ::prtcl::scheme::TEST<float, 3> test;
  test.require(scheme);

  test.load(scheme);

  test.Test();

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
