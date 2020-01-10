#pragma once

#include "common.hpp"

#include <prtcl/core/remove_cvref.hpp>

#include <string>
#include <utility>

#include <boost/hana/tuple.hpp>

namespace prtcl::gt::dsl {

struct procedure {
  std::string name;
};

using procedure_expr = term_expr<procedure>;

} // namespace prtcl::gt::dsl

namespace prtcl::gt::dsl::language {

template <typename... Exprs_>
auto procedure(std::string name_, Exprs_ &&... exprs_) {
  return procedure_expr{{name_}}(std::forward<Exprs_>(exprs_)...);
}

} // namespace prtcl::gt::dsl::language
