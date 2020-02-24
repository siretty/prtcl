#pragma once

#include "../ast.hpp"

namespace prtcl::gt {

struct field_less {
  static auto to_tuple(ast::init::field const &field_) {
    return std::make_tuple(
        field_.field_name,
        static_cast<std::underlying_type_t<ast::storage_qualifier>>(
            field_.storage),
        static_cast<std::underlying_type_t<ast::basic_type>>(
            field_.field_type.type),
        field_.field_type.shape);
  }

  bool
  operator()(ast::init::field const &lhs, ast::init::field const &rhs) const {
    return to_tuple(lhs) < to_tuple(rhs);
  }
};

} // namespace prtcl::gt
