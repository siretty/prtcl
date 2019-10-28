#include "prtcl/data/tensors.hpp"
#include <catch.hpp>

#include <prtcl/data/openmp/tensors.hpp>
#include <utility>

TEST_CASE("prtcl/data/openmp/tensors", "[prtcl][data][openmp][tensors]") {
  SECTION("Rank 0") {
    prtcl::data::tensors_t<int> t_data;
    t_data.reserve(100);
    ::prtcl::detail::resize_access::resize(t_data, 10ul);

    prtcl::data::openmp::tensors t{t_data};
    REQUIRE(0 == t.rank());
    REQUIRE(1 == t.component_count());
    REQUIRE(100 <= t.capacity());
    REQUIRE(10 == t.size());

    REQUIRE(nullptr != t.component_data(std::index_sequence<>{}));
  }

  SECTION("Rank 1") {
    SECTION("Extents (1)") {
      prtcl::data::tensors_t<int, 1> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(1 == t.rank());
      REQUIRE(1 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0>{}));
    }

    SECTION("Extents (2)") {
      prtcl::data::tensors_t<int, 2> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(1 == t.rank());
      REQUIRE(2 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<1>{}));
    }

    SECTION("Extents (3)") {
      prtcl::data::tensors_t<int, 3> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(1 == t.rank());
      REQUIRE(3 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<1>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<2>{}));
    }
  }

  SECTION("Rank 2") {
    SECTION("Extents (1,1)") {
      prtcl::data::tensors_t<int, 1, 1> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(2 == t.rank());
      REQUIRE(1 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 0>{}));
    }

    SECTION("Extents (1,2)") {
      prtcl::data::tensors_t<int, 1, 2> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(2 == t.rank());
      REQUIRE(2 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 1>{}));
    }

    SECTION("Extents (2,1)") {
      prtcl::data::tensors_t<int, 2, 1> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(2 == t.rank());
      REQUIRE(2 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<1, 0>{}));
    }

    SECTION("Extents (2,2)") {
      prtcl::data::tensors_t<int, 2, 2> t_data;
      t_data.reserve(100);
      ::prtcl::detail::resize_access::resize(t_data, 10ul);

      prtcl::data::openmp::tensors t{t_data};
      REQUIRE(2 == t.rank());
      REQUIRE(4 == t.component_count());
      REQUIRE(100 <= t.capacity());
      REQUIRE(10 == t.size());

      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<0, 1>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<1, 0>{}));
      REQUIRE(nullptr != t.component_data(std::index_sequence<1, 1>{}));
    }
  }
}
