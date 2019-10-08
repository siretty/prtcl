#include <catch.hpp>

#include <string>

#include <prtcl/expr/grammar/scalar.hpp>

#include <boost/proto/proto.hpp>

TEST_CASE("prtcl::expr::grammar scalar", "[prtcl][expr]") {
  using namespace prtcl::expr;

  active_index_term i;
  neighbour_index_term j;

  uniform_scalar_term<std::string> u{"u"};
  varying_scalar_term<std::string> v{"v"};

  REQUIRE(boost::proto::matches<decltype(2.f * u[i] + v[j] - u[i] * v[j] / u[i] +
                                         (+u[j]) + (-u[i])),
                                Scalar>::value);

  REQUIRE(!boost::proto::matches<decltype(u[i] % v[j]), Scalar>::value);
  REQUIRE(!boost::proto::matches<decltype(u(v)), Scalar>::value);

  REQUIRE(boost::proto::matches<decltype(u[i] = v[j] * v[i]),
                                ScalarAssignment>::value);
  REQUIRE(boost::proto::matches<decltype(v[i] = u[j] * v[i]),
                                ScalarAssignment>::value);

  REQUIRE(!boost::proto::matches<decltype(u[j] = v[j] * v[i]),
                                 ScalarAssignment>::value);
  REQUIRE(!boost::proto::matches<decltype(v[j] = u[j] * v[i]),
                                 ScalarAssignment>::value);

  REQUIRE(!boost::proto::matches<decltype(u = v), ScalarAssignment>::value);
  REQUIRE(!boost::proto::matches<decltype(v = u), ScalarAssignment>::value);

  REQUIRE(
      !boost::proto::matches<decltype((u * u) = v), ScalarAssignment>::value);
  REQUIRE(
      !boost::proto::matches<decltype((u * v) = u), ScalarAssignment>::value);
}

// vim: set foldmethod=marker:
