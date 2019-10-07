#include <catch.hpp>

#include <string>

#include <prtcl/expr/grammar/vector.hpp>

#include <boost/proto/proto.hpp>

TEST_CASE("prtcl::expr::grammar vector", "[prtcl][expr]") {
  using namespace prtcl::expr;

  active_index_term i;
  neighbour_index_term j;

  uniform_scalar_term<std::string> a{"a"};
  uniform_vector_term<std::string> u{"u"};
  varying_vector_term<std::string> v{"v"};

  REQUIRE(boost::proto::matches<decltype(u[i] + v[j] - u[i] * v[j] / u[i] +
                                         (+u[j]) + (-u[i]) + a[i] * u[j] +
                                         u[i] * a[j] + (a[i] + u[j])),
                                Vector>::value);

  REQUIRE(!boost::proto::matches<decltype(u[i] % v[j]), Vector>::value);
  REQUIRE(!boost::proto::matches<decltype(u(v)), Vector>::value);

  REQUIRE(boost::proto::matches<decltype(u[i] = v[i]), VectorAssignment>::value);
  REQUIRE(boost::proto::matches<decltype(v[i] = u[j]), VectorAssignment>::value);

  REQUIRE(!boost::proto::matches<decltype(u[j] = v[i]), VectorAssignment>::value);
  REQUIRE(!boost::proto::matches<decltype(v[j] = u[j]), VectorAssignment>::value);

  REQUIRE(!boost::proto::matches<decltype(u = v), VectorAssignment>::value);
  REQUIRE(!boost::proto::matches<decltype(v = u), VectorAssignment>::value);

  REQUIRE(
      !boost::proto::matches<decltype((a * u) = v), VectorAssignment>::value);
  REQUIRE(
      !boost::proto::matches<decltype((a * v) = u), VectorAssignment>::value);
}

// vim: set foldmethod=marker:
