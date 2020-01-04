#include <catch.hpp>

#include <algorithm>

#include <prtcl/data/host/grouped_uniform_grid.hpp>
#include <prtcl/openmp_neighbourhood.hpp>
#include <prtcl/scheme/prtcl_tests.hpp>

#include <boost/range/algorithm_ext.hpp>
#include <boost/range/irange.hpp>

TEST_CASE("prtcl/scheme/prtcl_tests", "[prtcl][scheme][prtcl_tests]") {
  using namespace ::prtcl::expr_literals;

  using scalar_type = float;
  constexpr size_t vector_extent = 3;

  ::prtcl::data::scheme<scalar_type, vector_extent> data;

  auto &m1 = data.add_group("m1").set_type("main").resize(10000);
  m1.add("position"_vvf);
  auto &m2 = data.add_group("m2").set_type("main").resize(10000);
  m2.add("position"_vvf);
  auto &o1 = data.add_group("o1").set_type("other").resize(10);
  o1.add("position"_vvf);

  ::prtcl::openmp_neighbourhood<
      ::prtcl::grouped_uniform_grid<scalar_type, vector_extent>>
      neighbourhood;
  neighbourhood.set_radius(1);
  neighbourhood.load(data);

  ::prtcl::scheme::prtcl_tests<scalar_type, vector_extent> scheme;
  scheme.require(data);

  using vector_type = decltype(scheme)::vector_type;

  { // initialization
    auto m1v = m1.get("varying"_vsf);
    auto m1p = m1.get("position"_vvf);
    for (size_t i = 0; i < m1.size(); ++i) {
      m1v[i] = static_cast<scalar_type>(i);
      m1p[i].setZero();
    }
  }

  { // initialization
    auto m2v = m2.get("varying"_vsf);
    auto m2p = m2.get("position"_vvf);
    for (size_t i = 0; i < m2.size(); ++i) {
      m2v[i] = -static_cast<scalar_type>(i);
      m2p[i].setZero();
    }
  }

  { // initialization
    auto o1p = o1.get("position"_vvf);
    for (size_t i = 0; i < o1.size(); ++i) {
      o1p[i].setZero();
      o1p[i][0] = static_cast<scalar_type>(i) / (o1.size() - 1) -
                  static_cast<scalar_type>(.5);
    }
  }

  neighbourhood.update();
  scheme.load(data);

  SECTION("reduce_sum") {
    scheme.reduce_sum(neighbourhood);

    REQUIRE(
        Approx(
            static_cast<scalar_type>(m1.size()) / 2 -
            static_cast<scalar_type>(.5))
            .epsilon(static_cast<scalar_type>(.001)) ==
        m1.get("uniform"_usf)[0]);

    REQUIRE(
        Approx(
            -static_cast<scalar_type>(m2.size()) / 2 +
            static_cast<scalar_type>(.5))
            .epsilon(static_cast<scalar_type>(.001)) ==
        m2.get("uniform"_usf)[0]);

    REQUIRE(
        Approx(static_cast<scalar_type>(0))
            .scale(1)
            .epsilon(static_cast<scalar_type>(.001)) ==
        data.get("global"_gsf)[0]);
  }

  SECTION("reduce_sum_neighbours") {
    scheme.reduce_sum_neighbours(neighbourhood);

    REQUIRE(
        Approx(0).scale(1).epsilon(static_cast<scalar_type>(.01)) ==
        m1.get("uniform"_usf)[0]);

    REQUIRE((m1.size() + m2.size()) * o1.size() == data.get("global"_gsf)[0]);
  }

  SECTION("call_norm") {
    auto invalid_length = [](auto a_) {
      return a_.norm() < static_cast<scalar_type>(0.001) ||
             (a_.norm() > 0.99f && a_.norm() < 1.01f);
    };

    auto m1a = m1.get("vv_a"_vvf);
    for (size_t i = 0; i < m1.size(); ++i) {
      vector_type tmp = vector_type::Zero();
      while (invalid_length(tmp))
        tmp = 10 * vector_type::Random();
      m1a[i] = tmp;
    }

    REQUIRE(std::all_of(m1a.data(), m1a.data() + m1a.size(), [&](auto a_) {
      return not invalid_length(a_);
    }));

    scheme.call_norm(neighbourhood);

    auto m1v = m1.get("varying"_vsf);
    auto indices = boost::irange(0UL, m1.size());
    REQUIRE(std::all_of(indices.begin(), indices.end(), [&](auto i_) {
      return m1a[i_].norm() == m1v[i_];
    }));
  }
}
