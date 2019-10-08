#pragma once

#include "functor_field_transform.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

template <typename FieldTerm, typename IndexTerm, typename FieldTransform>
struct ForModifiedFieldOf
    : boost::proto::or_<
          // match assignments
          boost::proto::when<boost::proto::assign<
                                 boost::proto::subscript<FieldTerm, IndexTerm>,
                                 boost::proto::_>,
                             FieldTransform(
                                 boost::proto::_child0(boost::proto::_child0))>,
          // match accumulates
          boost::proto::when<boost::proto::plus_assign<
                                 boost::proto::subscript<FieldTerm, IndexTerm>,
                                 boost::proto::_>,
                             FieldTransform(
                                 boost::proto::_child0(boost::proto::_child0))>,
          // match desired subscripted field
          boost::proto::when<boost::proto::subscript<FieldTerm, IndexTerm>,
                             FieldTransform(boost::proto::_child0)>> {};

} // namespace detail

template <typename FieldTerm, typename IndexTerm, typename Expr,
          typename Functor>
void for_modified_field(Expr const &e, Functor f) {
  detail::ForModifiedFieldOf<FieldTerm, IndexTerm,
                             detail::FunctorFieldTransform>
      transform;
  transform(e, 0, f);
}

} // namespace prtcl::expr
