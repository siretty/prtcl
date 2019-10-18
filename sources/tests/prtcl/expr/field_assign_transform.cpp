#include <catch.hpp>

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/field_assign_transform.hpp>
#include <prtcl/tags.hpp>

#include <cstddef>

#include <boost/yap/yap.hpp>

TEST_CASE("prtcl/expr/field_assign_transform",
          "[prtcl][expr][transform][field_assign_transform]") {
  using namespace prtcl;

  struct mock_settable_value {
    void set(size_t index_, int value_) const {
      this->index = index_;
      this->value = value_;
    }

    mutable int value;
    mutable size_t index;
  };

  struct mock_gettable_value {
    int get(size_t index_) const {
      this->index = index_;
      return this->value;
    }

    int value;
    mutable size_t index = 0;
  };

  auto transform = expr::field_assign_transform{12, 34};

  {
    expr::field_term<tag::uniform, tag::scalar, tag::active,
                     mock_settable_value>
        us_set;
    boost::yap::transform((us_set = 5678), transform);
    REQUIRE(12 == us_set.value().value.index);
    REQUIRE(5678 == us_set.value().value.value);

    expr::field_term<tag::varying, tag::scalar, tag::active,
                     mock_gettable_value>
        vs_active_get{{56}};
    boost::yap::transform((us_set = vs_active_get), transform);
    REQUIRE(12 == us_set.value().value.index);
    REQUIRE(56 == us_set.value().value.value);
    REQUIRE(12 == vs_active_get.value().value.index);

    expr::field_term<tag::varying, tag::scalar, tag::passive,
                     mock_gettable_value>
        vs_passive_get{{78}};
    boost::yap::transform((us_set = vs_passive_get), transform);
    REQUIRE(12 == us_set.value().value.index);
    REQUIRE(78 == us_set.value().value.value);
    REQUIRE(34 == vs_passive_get.value().value.index);

    boost::yap::transform((us_set = vs_active_get + vs_passive_get), transform);
    REQUIRE(12 == us_set.value().value.index);
    REQUIRE(56 + 78 == us_set.value().value.value);
    REQUIRE(12 == vs_active_get.value().value.index);
    REQUIRE(34 == vs_passive_get.value().value.index);
  }
}
