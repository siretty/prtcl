#pragma once

#include "../tags.hpp"

#include <type_traits>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename KindTag, typename TypeTag, typename GroupTag,
          typename AccessTag, typename Value>
struct field {
  static_assert(tag::is_kind_tag_v<KindTag>, "KindTag is invalid");
  static_assert(tag::is_type_tag_v<TypeTag>, "TypeTag is invalid");
  static_assert(tag::is_group_tag_v<GroupTag> or
                    tag::is_unspecified_tag_v<GroupTag>,
                "GroupTag is invalid");
  static_assert(tag::is_access_tag_v<AccessTag> or
                    tag::is_unspecified_tag_v<AccessTag>,
                "AccessTag is invalid");

  using kind_tag = KindTag;
  using type_tag = TypeTag;
  using group_tag = GroupTag;
  using access_tag = AccessTag;
  using value_type = Value;

  Value value;

  friend std::ostream &operator<<(std::ostream &s, field const &field_) {
    return s << "field<" << kind_tag{} << ", " << type_tag{} << ", "
             << group_tag{} << ", " << access_tag{} << ">{" << field_.value
             << "}";
  }
};

template <typename KT, typename TT, typename GT, typename AT, typename V>
using field_term =
    boost::yap::terminal<boost::yap::expression, field<KT, TT, GT, AT, V>>;

template <typename> struct is_field : std::false_type {};
template <typename KT, typename TT, typename GT, typename AT, typename V>
struct is_field<field<KT, TT, GT, AT, V>> : std::true_type {};

template <typename V>
using gscalar =
    field_term<tag::global, tag::scalar, tag::unspecified, tag::unspecified, V>;

template <typename V>
using gvector =
    field_term<tag::global, tag::vector, tag::unspecified, tag::unspecified, V>;

template <typename V>
using gmatrix =
    field_term<tag::global, tag::matrix, tag::unspecified, tag::unspecified, V>;

template <typename V>
using uscalar = field_term<tag::uniform, tag::scalar, tag::unspecified,
                           tag::unspecified, V>;

template <typename V>
using uvector = field_term<tag::uniform, tag::vector, tag::unspecified,
                           tag::unspecified, V>;

template <typename V>
using umatrix = field_term<tag::uniform, tag::matrix, tag::unspecified,
                           tag::unspecified, V>;

template <typename V>
using vscalar = field_term<tag::varying, tag::scalar, tag::unspecified,
                           tag::unspecified, V>;

template <typename V>
using vvector = field_term<tag::varying, tag::vector, tag::unspecified,
                           tag::unspecified, V>;

template <typename V>
using vmatrix = field_term<tag::varying, tag::matrix, tag::unspecified,
                           tag::unspecified, V>;

} // namespace prtcl::expr
