#pragma once

#include "functor_field_transform.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

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
