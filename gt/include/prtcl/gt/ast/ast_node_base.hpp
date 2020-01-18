#pragma once

#include <memory>

namespace prtcl::gt::ast {

class ast_node_base {
public:
  virtual ~ast_node_base() {}

public:
  template <typename NodeType_> auto *as_ptr(bool throw_ = false) {
    // dispatch to the const-version
    return const_cast<NodeType_ *>(
        static_cast<ast_node_base const *>(this)->as_ptr<NodeType_>(throw_));
  }

  template <typename NodeType_> auto *as_ptr(bool throw_ = false) const {
    if (throw_)
      return std::addressof(dynamic_cast<NodeType_ const &>(*this));
    else
      return dynamic_cast<NodeType_ const *>(this);
  }

public:
  virtual char const *ast_node_name() const = 0;

public:
  ast_node_base() = default;

  ast_node_base(ast_node_base const &) = delete;
  ast_node_base &operator=(ast_node_base const &) = delete;

  ast_node_base(ast_node_base &&) = default;
  ast_node_base &operator=(ast_node_base &&) = default;
};

} // namespace prtcl::gt::ast
