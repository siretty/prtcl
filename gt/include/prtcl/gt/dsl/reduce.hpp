#pragma once

#include "common.hpp"

#include <string>

namespace prtcl::gt::dsl {

struct reduce {
  std::string name;
};

using reduce_expr = term_expr<reduce>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename Arg_> auto reduce_max(Arg_ &&arg_) {
  return reduce_expr{{"max"}}(std::forward<Arg_>(arg_));
}

template <typename Arg_> auto reduce_min(Arg_ &&arg_) {
  return reduce_expr{{"min"}}(std::forward<Arg_>(arg_));
}

} // namespace prtcl::gt::dsl::language
