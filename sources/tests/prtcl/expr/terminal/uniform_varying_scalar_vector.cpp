#include <catch.hpp>

#include <string>

#include <prtcl/expr/terminal/uniform_scalar.hpp>
#include <prtcl/expr/terminal/uniform_vector.hpp>
#include <prtcl/expr/terminal/varying_scalar.hpp>
#include <prtcl/expr/terminal/varying_vector.hpp>

#include <boost/proto/proto.hpp>

TEST_CASE("prtcl::expr::terminal::[uniform,varying]_[scalar,vector]",
          "[prtcl][expr]") {
  using namespace prtcl::expr;

  uniform_scalar_term<std::string> us{"us"};
  REQUIRE(boost::proto::value(us).value == "us");
  REQUIRE(boost::proto::matches<decltype(us), UniformScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(us), UniformVectorTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(us), VaryingScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(us), VaryingVectorTerm>::value);

  uniform_vector_term<std::string> uv{"uv"};
  REQUIRE(boost::proto::value(uv).value == "uv");
  REQUIRE(!boost::proto::matches<decltype(uv), UniformScalarTerm>::value);
  REQUIRE(boost::proto::matches<decltype(uv), UniformVectorTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(uv), VaryingScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(uv), VaryingVectorTerm>::value);

  varying_scalar_term<std::string> vs{"vs"};
  REQUIRE(boost::proto::value(vs).value == "vs");
  REQUIRE(!boost::proto::matches<decltype(vs), UniformScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(vs), UniformVectorTerm>::value);
  REQUIRE(boost::proto::matches<decltype(vs), VaryingScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(vs), VaryingVectorTerm>::value);

  varying_vector_term<std::string> vv{"vv"};
  REQUIRE(boost::proto::value(vv).value == "vv");
  REQUIRE(!boost::proto::matches<decltype(vv), UniformScalarTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(vv), UniformVectorTerm>::value);
  REQUIRE(!boost::proto::matches<decltype(vv), VaryingScalarTerm>::value);
  REQUIRE(boost::proto::matches<decltype(vv), VaryingVectorTerm>::value);
}

// vim: set foldmethod=marker:
