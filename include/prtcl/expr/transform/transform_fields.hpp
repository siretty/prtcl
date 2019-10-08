#pragma once

#include "../../data/group_data.hpp"
#include "../grammar/field.hpp"
#include "../grammar/index.hpp"

#include "../terminal/varying_scalar.hpp"

#include <cstddef>

#include <boost/proto/proto.hpp>

namespace prtcl::expr {

namespace detail {

template <typename FieldTransform>
struct TransformFields
    : boost::proto::or_<
          // match desired subscripted field
          boost::proto::when<
              boost::proto::subscript<AnyFieldTerm, AnyIndexTerm>,
              boost::proto::_make_subscript(
                  FieldTransform(boost::proto::_value(boost::proto::_child0),
                                 boost::proto::_value(boost::proto::_child1),
                                 boost::proto::_data),
                  boost::proto::_child1)>,
          // swallow terminals
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_>,
          // recurse into non-terminals
          boost::proto::when<
              boost::proto::nary_expr<boost::proto::_,
                                      boost::proto::vararg<boost::proto::_>>,
              boost::proto::_default<TransformFields<FieldTransform>>>> {};

template <typename FieldTransform>
struct TransformSubscript
    : boost::proto::or_<
          // match desired subscripted field
          boost::proto::when<
              boost::proto::subscript<AnyFieldTerm, AnyIndexTerm>,
              FieldTransform(boost::proto::_value(boost::proto::_child0),
                             boost::proto::_value(boost::proto::_child1),
                             boost::proto::_data)>,
          // swallow terminals
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_value>,
          // recurse into non-terminals
          boost::proto::when<
              boost::proto::nary_expr<boost::proto::_,
                                      boost::proto::vararg<boost::proto::_>>,
              boost::proto::_default<TransformSubscript<FieldTransform>>>> {};

template <typename LHSFieldTransform, typename RHSFieldTransform>
struct AssignTransformFields
    : boost::proto::when<
          boost::proto::assign<boost::proto::_, boost::proto::_>,
          boost::proto::_make_assign(
              boost::proto::call<TransformFields<LHSFieldTransform>(
                  boost::proto::_child0, boost::proto::_state,
                  boost::proto::_data)>,
              boost::proto::call<TransformFields<RHSFieldTransform>(
                  boost::proto::_child1, boost::proto::_state,
                  boost::proto::_data)>)> {};

} // namespace detail

} // namespace prtcl::expr
