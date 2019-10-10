#pragma once

#include "../tags.hpp"
#include "expr.hpp"

#include <ostream>
#include <string>
#include <type_traits>

#include <boost/lexical_cast.hpp>
#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

template <typename KindTag, typename TypeTag, typename GroupTag,
          typename DataType>
struct field_data_value {
  using kind_tag = KindTag;
  using type_tag = TypeTag;
  using group_tag = GroupTag;
  using data_type = DataType;

  data_type data;

  static_assert(tag::is_kind_tag_v<KindTag>, "invalid KindTag");
  static_assert(tag::is_type_tag_v<TypeTag>, "invalid TypeTag");
  static_assert(tag::is_group_tag_v<GroupTag>, "invalid GroupTag");

  friend std::ostream &operator<<(std::ostream &s, field_data_value const &v) {
    s << "field_data<" << kind_tag{} << ", " << type_tag{} << ", "
      << group_tag{} << ">(";
    s << boost::lexical_cast<std::string>(v.data);
    return s << ")";
  }
};

// ============================================================

template <typename T> struct is_field_data_value : std::false_type {};

template <typename KT, typename TT, typename GT, typename DT>
struct is_field_data_value<field_data_value<KT, TT, GT, DT>> : std::true_type {
};

template <typename T>
constexpr bool is_field_data_value_v = is_field_data_value<T>::value;

// ============================================================

template <typename KT, typename TT, typename GT, typename DT>
using field_data = expr<
    typename boost::proto::terminal<field_data_value<KT, TT, GT, DT>>::type>;

struct FieldData
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_field_data_value<boost::proto::_value>()>> {};

} // namespace expression
} // namespace prtcl
