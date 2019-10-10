#include <catch.hpp>

#include "../format_cxx_type.hpp"

#include <prtcl/data/group_data.hpp>
#include <prtcl/expression/eval_context.hpp>
#include <prtcl/expression/field_data.hpp>
#include <prtcl/expression/field_name.hpp>
#include <prtcl/expression/group.hpp>
#include <prtcl/expression/transform/access_all_fields.hpp>
#include <prtcl/expression/transform/index_all_field_names.hpp>
#include <prtcl/expression/transform/resolve_all_fields.hpp>
#include <prtcl/tags.hpp>

#include <functional>
#include <sstream>
#include <string>

#include <boost/type_index.hpp>

#include <eigen3/Eigen/Eigen>

TEST_CASE("prtcl::expression::field_data", "[prtcl][expression][field_data]") {
  using namespace prtcl;
  using namespace prtcl::expression;

  using T = float;
  constexpr size_t N = 3;

  auto str = [](auto f) {
    std::ostringstream s;
    s << f;
    return s.str();
  };

  REQUIRE("field_name<uniform, scalar>(hello world)" ==
          str(field_name_value<tag::uniform, tag::scalar>{"hello world"}));

  std::function<void(std::ostream &)> run_after_scope;

  struct {
    group_data<T, N> active, passive;
  } gds;

  auto init_group = [](auto &group) {
    group.resize(10);
    group.add_uniform_scalar("aaa");
    group.add_varying_scalar("bbb");
    group.add_uniform_vector("ccc");
    group.add_varying_vector("ddd");
    group.add_uniform_scalar("eee");
    group.add_uniform_vector("fff");
  };

  init_group(gds.active);
  init_group(gds.passive);

  auto buffer_gds = [](auto &gds) {
    struct {
      typename result_of::get_buffer<decltype(gds.active), tag::host>::type
          active,
          passive;
    } gbs{get_buffer(gds.active, tag::host{}),
          get_buffer(gds.passive, tag::host{})};
    return gbs;
  };

  auto gbs = buffer_gds(gds);

  { // create local variables for the field names
    active_group i;
    passive_group j;

    auto a = make_uniform_scalar_field_name("aaa");
    auto b = make_varying_scalar_field_name("bbb");
    auto c = make_uniform_vector_field_name("ccc");
    auto d = make_varying_vector_field_name("ddd");
    auto e = make_uniform_scalar_field_name("eee");
    auto f = make_uniform_vector_field_name("fff");

    // auto expr_fn = boost::proto::deep_copy(
    //    a[i] = 101 * (b[i] * a[j] - c[i] * d[j] * a[i] * d[j] * c[i] / b[j] +
    //                  e[i] - f[j]));
    // auto expr_fn = boost::proto::deep_copy(b[i] = b[j] * b[i]);
    // auto expr_fn = boost::proto::deep_copy(d[i] = d[j] * d[i]);
    auto expr_fn = boost::proto::deep_copy(d[i] = b[i] * d[j] + b[j] * d[i]);

    std::cout << "expr_fn = ";
    boost::proto::display_expr(expr_fn, std::cout);
    display_cxx_type(expr_fn, std::cout);

    auto expr_fd = index_all_field_names{}(expr_fn);

    std::cout << "expr_fd = ";
    boost::proto::display_expr(expr_fd, std::cout);
    display_cxx_type(expr_fd, std::cout);

    auto expr_fb = resolve_all_fields{}(expr_fd, 0, gbs);

    std::cout << "expr_fb = ";
    boost::proto::display_expr(expr_fb, std::cout);
    display_cxx_type(expr_fb, std::cout);

    auto expr_fa = access_all_fields{}(expr_fb, 0, std::make_tuple());

    std::cout << "expr_fa = ";
    boost::proto::display_expr(expr_fa, std::cout);
    display_cxx_type(expr_fa, std::cout);

    // make sure no stack memory is accessed by moving accessing the
    // expressions into a seperate stack frame (testing done with
    // AddressSanitizer)
    run_after_scope = [=](auto &out) {
      out << "expr_fn = ";
      boost::proto::display_expr(expr_fn, out);
      display_cxx_type(expr_fn, out);

      out << "expr_fd = ";
      boost::proto::display_expr(expr_fd, out);
      display_cxx_type(expr_fd, out);

      std::cout << "expr_fb = ";
      boost::proto::display_expr(expr_fb, std::cout);
      display_cxx_type(expr_fb, std::cout);

      std::cout << "expr_fa = ";
      boost::proto::display_expr(expr_fa, std::cout);
      display_cxx_type(expr_fa, std::cout);

      eval_context<T, Eigen::Array<T, N, 1>> ctx;
      ctx.active = 0;
      ctx.passive = 1;

      boost::proto::eval(expr_fa, ctx);
    };
  }

  std::cout << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  run_after_scope(std::cout);
}
