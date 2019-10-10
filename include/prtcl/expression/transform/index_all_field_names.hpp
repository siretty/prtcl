#pragma once

#include "../../meta/is_any_of.hpp"
#include "../field_data.hpp"
#include "../field_name.hpp"
#include "../group.hpp"

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

struct index_field_name : boost::proto::callable {
  template <typename> struct result;

  template <typename T>
  using rcvr_t = std::remove_cv_t<std::remove_reference_t<T>>;

  template <typename This, typename FN, typename GT>
  struct result<This(FN, GT)> {
    using type =
        field_data<typename rcvr_t<FN>::kind_tag, typename rcvr_t<FN>::type_tag,
                   rcvr_t<GT>, std::string>;
  };

  template <typename FN, typename GT>
  typename result<index_field_name(FN, GT)>::type operator()(FN const &fn,
                                                             GT const &) {
    using kind_tag = typename rcvr_t<FN>::kind_tag;
    if constexpr (is_any_of_v<kind_tag, tag::uniform>)
      return {0, fn.name};
    else
      return {fn.name};
  }
};

struct index_all_field_names
    : boost::proto::or_<
          // Subscripts of a FieldName by a Group are collapsed into a
          // field_data object.
          boost::proto::when<boost::proto::subscript<FieldName, Group>,
                             index_field_name(
                                 boost::proto::_value(boost::proto::_child0),
                                 boost::proto::_value(boost::proto::_child1))>,
          // Terminals are stored by value and are not modified in this step.
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_byval(boost::proto::_)>,
          // Recurse into any Non-Terminals.
          boost::proto::nary_expr<
              boost::proto::_, boost::proto::vararg<index_all_field_names>>> {};

} // namespace expression
} // namespace prtcl
