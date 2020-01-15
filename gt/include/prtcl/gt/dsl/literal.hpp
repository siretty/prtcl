#pragma once

#include "common.hpp"

#include <initializer_list>
#include <string>

namespace prtcl::gt::dsl {

struct literal {
  std::string value;
  nd_dtype dtype;
};

using literal_expr = term_expr<literal>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

inline auto rlit(long double value_) {
  return literal_expr{{std::to_string(value_), nd_dtype::real}};
}

inline auto ilit(long long int value_) {
  return literal_expr{{std::to_string(value_), nd_dtype::integer}};
}

inline auto blit(bool value_) {
  return literal_expr{{std::to_string(value_), nd_dtype::boolean}};
}

} // namespace prtcl::gt::dsl::language
