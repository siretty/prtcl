#pragma once

#include "../tags.hpp"
#include "expr.hpp"

#include <ostream>
#include <string>
#include <type_traits>

#include <boost/proto/proto.hpp>

namespace prtcl {
namespace expression {

template <typename KindTag, typename TypeTag> struct field_name_value {
  std::string name;

  using kind_tag = KindTag;
  using type_tag = TypeTag;

  static_assert(tag::is_kind_tag_v<KindTag>, "invalid KindTag");
  static_assert(tag::is_type_tag_v<TypeTag>, "invalid TypeTag");

  friend std::ostream &operator<<(std::ostream &s, field_name_value const &v) {
    return s << "field_name<" << kind_tag{} << ", " << type_tag{} << ">("
             << v.name << ")";
  }
};

// ============================================================

template <typename T> struct is_field_name_value : std::false_type {};

template <typename KT, typename TT>
struct is_field_name_value<field_name_value<KT, TT>> : std::true_type {};

template <typename T>
constexpr bool is_field_name_value_v = is_field_name_value<T>::value;

// ============================================================

template <typename KT, typename TT>
using field_name =
    expr<typename boost::proto::terminal<field_name_value<KT, TT>>::type>;

struct FieldName
    : boost::proto::and_<
          boost::proto::terminal<boost::proto::_>,
          boost::proto::if_<is_field_name_value<boost::proto::_value>()>> {};

// ============================================================

inline field_name<tag::uniform, tag::scalar>
make_uniform_scalar_field_name(std::string name) {
  return {name};
}

inline field_name<tag::uniform, tag::vector>
make_uniform_vector_field_name(std::string name) {
  return {name};
}

inline field_name<tag::varying, tag::scalar>
make_varying_scalar_field_name(std::string name) {
  return {name};
}

inline field_name<tag::varying, tag::vector>
make_varying_vector_field_name(std::string name) {
  return {name};
}

} // namespace expression
} // namespace prtcl
