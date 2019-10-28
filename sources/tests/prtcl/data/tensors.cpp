#include <catch.hpp>

#include <prtcl/data/tensors.hpp>

TEST_CASE("prtcl/data/tensors", "[prtcl][data][tensors]") {
  auto capacity_size_tests = [](auto &t) {
    t.reserve(100);
    REQUIRE(100 <= t.capacity());

    ::prtcl::detail::resize_access::resize(t, 10ul);
    REQUIRE(10 == t.size());

    ::prtcl::detail::resize_access::clear(t);
    REQUIRE(0 == t.size());
  };

  SECTION("Rank 0") {
    prtcl::data::tensors_t<int> t;
    REQUIRE(0 == t.rank());
    REQUIRE(1 == t.component_count());

    capacity_size_tests(t);
  }

  SECTION("Rank 1") {
    SECTION("Extents (1)") {
      prtcl::data::tensors_t<int, 1> t;
      REQUIRE(1 == t.rank());
      REQUIRE(1 == t.component_count());

      capacity_size_tests(t);
    }

    SECTION("Extents (2)") {
      prtcl::data::tensors_t<int, 2> t;
      REQUIRE(1 == t.rank());
      REQUIRE(2 == t.component_count());

      capacity_size_tests(t);
    }

    SECTION("Extents (3)") {
      prtcl::data::tensors_t<int, 3> t;
      REQUIRE(1 == t.rank());
      REQUIRE(3 == t.component_count());

      capacity_size_tests(t);
    }
  }

  SECTION("Rank 2") {
    SECTION("Extents (1,1)") {
      prtcl::data::tensors_t<int, 1, 1> t;
      REQUIRE(2 == t.rank());
      REQUIRE(1 == t.component_count());

      capacity_size_tests(t);
    }

    SECTION("Extents (1,2)") {
      prtcl::data::tensors_t<int, 1, 2> t;
      REQUIRE(2 == t.rank());
      REQUIRE(2 == t.component_count());

      capacity_size_tests(t);
    }

    SECTION("Extents (2,1)") {
      prtcl::data::tensors_t<int, 2, 1> t;
      REQUIRE(2 == t.rank());
      REQUIRE(2 == t.component_count());

      capacity_size_tests(t);
    }

    SECTION("Extents (2,2)") {
      prtcl::data::tensors_t<int, 2, 2> t;
      REQUIRE(2 == t.rank());
      REQUIRE(4 == t.component_count());

      capacity_size_tests(t);
    }
  }
}
