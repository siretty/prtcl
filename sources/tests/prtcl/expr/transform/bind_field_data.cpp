#include <catch.hpp>

#include <prtcl/data/group_data.hpp>
#include <prtcl/data/host/host_linear_access.hpp>
#include <prtcl/data/host/host_linear_data.hpp>

#include <prtcl/expr/terminal/active_index.hpp>
#include <prtcl/expr/terminal/neighbour_index.hpp>
#include <prtcl/expr/terminal/varying_scalar.hpp>

#include <prtcl/expr/transform/access_field_data.hpp>
#include <prtcl/expr/transform/bind_field_data.hpp>
#include <prtcl/expr/transform/buffer_field_data.hpp>
#include <prtcl/expr/transform/evaluate_assign.hpp>

#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/type_index.hpp>

TEST_CASE("prtcl::expr::bind_field_data", "[prtcl][expr]") {
  prtcl::expr::active_index_term i;
  prtcl::expr::neighbour_index_term j;

  prtcl::expr::varying_scalar_term<std::string> vs_a{"a"};
  prtcl::expr::varying_scalar_term<std::string> vs_b{"b"};
  prtcl::expr::varying_scalar_term<std::string> vs_c{"c"};

  prtcl::expr::varying_vector_term<std::string> vv_a{"a"};
  prtcl::expr::varying_vector_term<std::string> vv_b{"b"};
  prtcl::expr::varying_vector_term<std::string> vv_c{"c"};

  using T = float;
  constexpr size_t N = 3;

  std::vector<prtcl::group_data<T, N>> groups(2);

  for (auto &g : groups) {
    g.add_varying_scalar("a");
    g.add_varying_vector("a");
    g.add_varying_scalar("b");
    g.add_varying_vector("b");
    g.add_varying_scalar("c");
    g.add_varying_vector("c");

    g.resize(10);
  }

  for (size_t i_g = 0; i_g < groups.size(); ++i_g) {
    auto &g = groups[i_g];

    if (auto vs = g.get_varying_scalar("a"))
      vs->fill(1 + 10 * i_g);
    if (auto vs = g.get_varying_scalar("b"))
      vs->fill(2 + 10 * i_g);
    if (auto vs = g.get_varying_scalar("c"))
      vs->fill(3 + 10 * i_g);
  }

  auto scalar_expr = boost::proto::deep_copy(vs_a[i] = 4 * vs_b[i] * vs_b[j] +
                                                       vs_c[i] * vs_c[j]);
  // auto vector_expr = boost::proto::deep_copy(vv_a[i] = 4 * vv_b[i] * vv_b[j]
  // +
  //                                                     vv_c[i] * vv_c[j]);

  boost::proto::display_expr(scalar_expr);
  // boost::proto::display_expr(vector_expr);

  std::cout << "decltype(scalar_expr) = "
            << boost::typeindex::type_id<decltype(scalar_expr)>().pretty_name()
            << std::endl;

  auto scalar_expr_bound =
      prtcl::expr::bind_field_data(scalar_expr, groups[0], groups[1]);
  // auto vector_expr_bound =
  //    prtcl::expr::bind_field_data(vector_expr, groups[0], groups[1]);

  std::cout
      << "decltype(scalar_expr_bound) = "
      << boost::typeindex::type_id<decltype(scalar_expr_bound)>().pretty_name()
      << std::endl;

  boost::proto::display_expr(scalar_expr_bound);
  // boost::proto::display_expr(vector_expr_bound);

  auto scalar_expr_buffered = prtcl::expr::buffer_field_data(scalar_expr_bound);
  // auto vector_expr_buffered =
  // prtcl::expr::buffer_field_data(vector_expr_bound);

  std::cout << "decltype(scalar_expr_buffered) = "
            << boost::typeindex::type_id<decltype(scalar_expr_buffered)>()
                   .pretty_name()
            << std::endl;

  boost::proto::display_expr(scalar_expr_buffered);
  // boost::proto::display_expr(vector_expr_buffered);

  auto scalar_expr_access =
      prtcl::expr::access_field_data(scalar_expr_buffered);
  // auto vector_expr_access =
  //    prtcl::expr::access_field_data(vector_expr_buffered);

  std::cout
      << "decltype(scalar_expr_access) = "
      << boost::typeindex::type_id<decltype(scalar_expr_access)>().pretty_name()
      << std::endl;

  boost::proto::display_expr(scalar_expr_access);
  // boost::proto::display_expr(vector_expr_access);

  auto scalar_expr_result =
      prtcl::expr::evaluate_assign(scalar_expr_access, 0, 0);

  std::cout
      << "decltype(scalar_expr_result) = "
      << boost::typeindex::type_id<decltype(scalar_expr_result)>().pretty_name()
      << std::endl;

  std::cout << "scalar_expr_result = " << scalar_expr_result << std::endl;
  // boost::proto::display_expr(scalar_expr_result);
}
