#pragma once

#include <prtcl/gt/ast.hpp>

#include <prtcl/gt/misc/field_map.hpp>

namespace prtcl::gt {

namespace detail {

struct find_global_fields_impl {
  /// Match scheme ... { ... }.
  void operator()(ast::scheme const &node) {
    for (auto statement : node.statements)
      (*this)(statement);
  }

  /// Match scheme ... { global { ... } }.
  void operator()(ast::global const &node) {
    // TODO: context with error_handler
    // TODO: think about allowing multiple global { ... } nodes per scheme
    if (global_encountered)
      throw std::runtime_error{"only one global { ... } per scheme allowed"};

    for (auto &field : node.fields) {
      map.insert(field);
    }

    global_encountered = true;
  }

  /// Ignore scheme ... { groups ... { ... } }.
  void operator()(ast::groups const &) const {}

  /// Ignore scheme ... { procedure ... { ... } }.
  void operator()(ast::n_scheme::procedure const &) const {}

  // {{{ generic

  // TODO: refactor into seperate type

  template <typename... Ts_>
  void operator()(std::variant<Ts_...> const &node_) {
    std::visit(*this, node_);
  }

  template <typename T_> void operator()(value_ptr<T_> const &node_) {
    if (not node_)
      throw "empty value_ptr";
    (*this)(*node_);
  }

  void operator()(std::monostate) { throw "monostate in ast traversal"; }

  // }}}

public:
  field_map map;

  bool global_encountered = false;
};

} // namespace detail

auto find_global_fields(ast::scheme const &scheme) {
  detail::find_global_fields_impl impl;
  impl(scheme);
  return std::move(impl.map);
}

} // namespace prtcl::gt
