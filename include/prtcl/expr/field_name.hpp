#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <string>

#include <boost/yap/yap.hpp>

namespace prtcl::expr {

template <typename KT, typename TT> class field_name {
  static_assert(tag::is_kind_v<KT>);
  static_assert(tag::is_type_v<TT>);

public:
  field_name(std::string name_) : _name{name_} {}

  field_name(field_name const &) = default;
  field_name &operator=(field_name const &) = default;

  field_name(field_name &&) = default;
  field_name &operator=(field_name &&) = default;

private:
  std::string _name;
};

} // namespace prtcl::expr
