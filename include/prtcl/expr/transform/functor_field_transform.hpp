#pragma once

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

struct FunctorFieldTransform : boost::proto::transform<FunctorFieldTransform> {
  template <typename E, typename S, typename D>
  struct impl : boost::proto::transform_impl<E, S, D> {
    using result_type = E;

    result_type operator()(typename impl::expr_param expr,
                           typename impl::state_param,
                           typename impl::data_param data) const {
      data(boost::proto::value(expr));
      return expr;
    }
  };
};

} // namespace detail

} // namespace prtcl::expr
