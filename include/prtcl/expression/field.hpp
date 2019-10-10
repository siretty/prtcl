#pragma once

#include "../expr/prtcl_domain.hpp"
#include "../tags.hpp"

#include <ostream>
#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

template <typename KindTag, typename TypeTag,
          typename GroupTag = tag::unspecified>
struct field {
  using kind_tag = KindTag;
  using type_tag = TypeTag;
  using group_tag = GroupTag;

  static_assert(tag::is_kind_tag_v<KindTag>, "invalid KindTag");
  static_assert(tag::is_type_tag_v<TypeTag>, "invalid TypeTag");
  static_assert(tag::is_group_tag_v<GroupTag> ||
                    tag::is_unspecified_tag_v<GroupTag>,
                "invalid GroupTag");

  friend std::ostream &operator<<(std::ostream &s, field) {
    if constexpr (tag::is_unspecified_tag_v<group_tag>)
      return s << "field<" << kind_tag{} << ", " << type_tag{} << ">";
    else
      return s << "field<" << kind_tag{} << ", " << type_tag{} << ", "
               << group_tag{} << ">";
  }
};

template <typename T> struct is_field : std::false_type {};

template <typename KT, typename TT, typename GT>
struct is_field<field<KT, TT, GT>> : std::true_type {};

template <typename T> constexpr bool is_field_v = is_field<T>::value;

template <typename KT, typename TT, typename GT = tag::unspecified>
using field_term =
    expr::prtcl_expr<typename boost::proto::terminal<field<KT, TT, GT>>::type>;

} // namespace expression
} // namespace prtcl
