#pragma once

#include <prtcl/math/kernel/cubic_spline_kernel.hpp>
#include <prtcl/tag/call.hpp>

#include <boost/hana.hpp>

namespace prtcl::expr::openmp {

template <typename Kernel> constexpr auto make_call_map(Kernel &&kernel_) {
  using scalar_type = typename Kernel::scalar_type;
  using boost::hana::make_map, boost::hana::make_pair, boost::hana::type_c;
  return make_map(
      make_pair(type_c<tag::call::dot>,
                [](auto &&lhs, auto &&rhs) {
                  return std::forward<decltype(lhs)>(lhs).matrix().dot(
                      std::forward<decltype(rhs)>(rhs));
                }),
      make_pair(type_c<tag::call::norm>,
                [](auto &&arg) {
                  return std::forward<decltype(arg)>(arg).matrix().norm();
                }),
      make_pair(
          type_c<tag::call::norm_squared>,
          [](auto &&arg) {
            return std::forward<decltype(arg)>(arg).matrix().squaredNorm();
          }),
      make_pair(type_c<tag::call::normalized>,
                [](auto &&arg) {
                  return std::forward<decltype(arg)>(arg).matrix().normalized();
                }),
      make_pair(type_c<tag::call::max>,
                [](auto &&lhs, auto &&rhs) {
                  return std::max<scalar_type>(
                      std::forward<decltype(lhs)>(lhs),
                      std::forward<decltype(rhs)>(rhs));
                }),
      make_pair(type_c<tag::call::min>,
                [](auto &&lhs, auto &&rhs) {
                  return std::min<scalar_type>(
                      std::forward<decltype(lhs)>(lhs),
                      std::forward<decltype(rhs)>(rhs));
                }),
      make_pair(type_c<tag::call::kernel>,
                [W = std::forward<Kernel>(kernel_)](auto &&arg, auto h) {
                  return W.eval(arg, h);
                }),
      make_pair(type_c<tag::call::kernel_gradient>,
                [W = std::forward<Kernel>(kernel_)](auto &&arg, auto h) {
                  return W.evalgrad(arg, h);
                }));
}

} // namespace prtcl::expr::openmp
