#include <catch.hpp>

#include "../format_cxx_type.hpp"

#include <prtcl/expression/field_name.hpp>

#include <sstream>
#include <string>

#include <boost/type_index.hpp>

TEST_CASE("prtcl::expression::field_name", "[prtcl][expression][field_name]") {
  using namespace prtcl;
  using namespace prtcl::expression;

  auto str = [](auto f) { return (std::ostringstream{} << f).str(); };

  REQUIRE("field_name<uniform, scalar>(hello world)" ==
          str(field_name_value<tag::uniform, tag::scalar>{"hello world"}));

  std::function<std::string()> run_after_scope;

  { // create local variables for the field names
    auto a = make_uniform_scalar_field_name("aaa");
    auto b = make_varying_scalar_field_name("bbb");
    auto c = make_uniform_vector_field_name("ccc");
    auto d = make_varying_vector_field_name("ddd");

    auto expr = boost::proto::deep_copy(
        a[1] = (b[2] * a[3] - c[0] * d[1] * a[2] * d[3] * c[4] / b[4]));

    run_after_scope = [=]() {
      std::ostringstream s;
      s << format_cxx_type<decltype(expr)>() << '\n';
      boost::proto::display_expr(expr, s);
      return s.str();
    };
  }

  std::cout << run_after_scope() << std::endl;
}
