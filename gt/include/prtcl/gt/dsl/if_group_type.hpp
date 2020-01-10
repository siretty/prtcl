#pragma once

#include "common.hpp"

#include <string>

namespace prtcl::gt::dsl {

struct if_group_type {
  std::string group_type;
};

using if_group_type_expr = term_expr<if_group_type>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename... Exprs_>
auto if_group_type(std::string group_type_, Exprs_ &&... exprs_) {
  return if_group_type_expr{{group_type_}}(std::forward<Exprs_>(exprs_)...);
}

} // namespace prtcl::gt::dsl::language
