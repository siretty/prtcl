#include <catch.hpp>

#include "../format_cxx_type.hpp"

#include <prtcl/expression/field_name.hpp>
#include <prtcl/expression/group.hpp>
#include <prtcl/expression/transform/field_name_to_group_field.hpp>

#include <functional>
#include <sstream>
#include <string>

#include <boost/type_index.hpp>

TEST_CASE("prtcl::expression::group_field", "[prtcl][expression][field_name]") {
  using namespace prtcl;
  using namespace prtcl::expression;

  auto str = [](auto f) { return (std::ostringstream{} << f).str(); };

  REQUIRE("field_name<uniform, scalar>(hello world)" ==
          str(field_name_value<tag::uniform, tag::scalar>{"hello world"}));

  std::function<std::string()> run_after_scope;

  { // create local variables for the field names
    active_group i;
    passive_group j;

    auto a = make_uniform_scalar_field_name("aaa");
    auto b = make_varying_scalar_field_name("bbb");
    auto c = make_uniform_vector_field_name("ccc");
    auto d = make_varying_vector_field_name("ddd");

    auto expr_nf = boost::proto::deep_copy(
        a[i] = (b[i] * a[j] - c[i] * d[j] * a[i] * d[j] * c[i] / b[j]));

    auto expr_gf = field_name_to_group_field(expr_nf);

    std::cout << format_cxx_type<decltype(expr_gf)>() << '\n';
    boost::proto::display_expr(expr_gf, std::cout);

    run_after_scope = [=]() {
      std::ostringstream s;
      s << format_cxx_type<decltype(expr_nf)>() << '\n';
      boost::proto::display_expr(expr_nf, s);
      s << '\n';
      s << format_cxx_type<decltype(expr_gf)>() << '\n';
      boost::proto::display_expr(expr_gf, s);
      return s.str();
    };
  }

  std::cout << run_after_scope() << std::endl;
}
