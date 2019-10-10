#pragma once

#include "../field_name.hpp"
#include "../group.hpp"
#include "../group_field.hpp"

#include "../../meta/remove_cvref.hpp"

#include <string>
#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

struct MakeGroupField : boost::proto::callable {
  template <typename> struct result;

  template <typename This, typename FN, typename GT>
  struct result<This(FN, GT)> {
    using type = group_field<typename remove_cvref_t<FN>::kind_tag,
                             typename remove_cvref_t<FN>::type_tag,
                             remove_cvref_t<GT>, std::string>;
  };

  template <typename KT, typename TT, typename GT>
  auto operator()(field_name_value<KT, TT> const &fn, GT const &) {
    return group_field<KT, TT, GT, std::string>{fn.name};
  }
};

struct FieldNameToGroupField
    : boost::proto::or_<
          boost::proto::when<
              boost::proto::subscript<FieldName, boost::proto::_>,
              boost::proto::_byval(
                  MakeGroupField(boost::proto::_value(boost::proto::_child0),
                                 boost::proto::_value(boost::proto::_child1)))>,
          boost::proto::when<boost::proto::terminal<boost::proto::_>,
                             boost::proto::_>,
          boost::proto::when<
              boost::proto::nary_expr<boost::proto::_,
                                      boost::proto::vararg<boost::proto::_>>,
              boost::proto::_byval(
                  boost::proto::_default<FieldNameToGroupField>)>> {};

template <typename E> auto field_name_to_group_field(E e) {
  FieldNameToGroupField transform;
  return transform(e, 0, 0);
}

} // namespace expression
} // namespace prtcl
