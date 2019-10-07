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

template <typename FieldTerm, typename IndexTerm, typename FieldTransform>
struct ForEachIndexedFieldOf
    : boost::proto::or_<
          // match desired subscripted field
          boost::proto::when<boost::proto::subscript<FieldTerm, IndexTerm>,
                             FieldTransform(boost::proto::_child0)>,
          // swallow terminals
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_>,
          // recurse into non-terminals
          boost::proto::when<
              boost::proto::nary_expr<boost::proto::_,
                                      boost::proto::vararg<boost::proto::_>>,
              boost::proto::_default<ForEachIndexedFieldOf<FieldTerm, IndexTerm,
                                                           FieldTransform>>>> {
};

} // namespace detail

template <typename FieldTerm, typename IndexTerm, typename Expr,
          typename Functor>
void for_each_used_field(Expr const &e, Functor f) {
  detail::ForEachIndexedFieldOf<FieldTerm, IndexTerm,
                                detail::FunctorFieldTransform>
      transform;
  transform(e, 0, f);
}

} // namespace prtcl::expr
