#include <catch.hpp>

#include <prtcl/gt/function_literals.hpp>

#include <type_traits>

#include <boost/lexical_cast.hpp>

TEST_CASE("prtcl/gt/function", "[prtcl]") {
  using namespace ::prtcl::gt::function_literals;

  CHECK("test" == "test"_fun.name());
}
