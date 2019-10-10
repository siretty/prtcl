#pragma once

#include "../tags.hpp"

#include <ostream>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

template <typename KindTag, typename TypeTag, typename GroupTag, typename Data>
struct group_field_value {
  Data data;

  using kind_tag = KindTag;
  using type_tag = TypeTag;
  using group_tag = GroupTag;
  using data_type = Data;

  static_assert(tag::is_kind_tag_v<KindTag>, "invalid KindTag");
  static_assert(tag::is_type_tag_v<TypeTag>, "invalid TypeTag");
  static_assert(tag::is_group_tag_v<GroupTag>, "invalid GroupTag");

  friend std::ostream &operator<<(std::ostream &s, group_field_value const &v) {
    s << "group_field<" << kind_tag{} << ", " << type_tag{} << ", "
      << group_tag{} << ">(";
    if constexpr (std::is_convertible<Data, std::string>::value)
      s << static_cast<std::string>(v.data);
    else
      s << "...";
    return s << ")";
  }
};

// ============================================================

template <typename KT, typename TT, typename GT, typename D>
using group_field =
    typename boost::proto::terminal<group_field_value<KT, TT, GT, D>>::type;

// ============================================================

template <typename T> struct is_group_field_value : std::false_type {};

template <typename KT, typename TT, typename GT, typename D>
struct is_group_field_value<group_field_value<KT, TT, GT, D>> : std::true_type {
};

template <typename T>
constexpr bool is_group_field_value_v = is_group_field_value<T>::value;

} // namespace expression
} // namespace prtcl
