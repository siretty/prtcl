#pragma once

#include "prtcl/gt/bits/ast_define.hpp"
#include <prtcl/core/overloaded.hpp>
#include <prtcl/core/range.hpp>

#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/misc/field_map.hpp>

#include <tuple>
#include <unordered_map>
#include <variant>

namespace prtcl::gt {

struct groups_map {
  size_t size() const { return name_to_groups.size(); }

  bool empty() const { return size() == 0; }

  auto all() const { return core::make_iterator_range(name_to_groups); }

  auto &groups_for_name(std::string name) {
    auto &n2g = name_to_groups;

    if (auto it = n2g.find(name); it != n2g.end()) {
      return it->second;
    } else {
      throw std::runtime_error{"groups name not mapped to groups"};
    }
  }

  struct per_groups {
    ast::n_groups::select_expression select;
    field_map uniform_fields;
    field_map varying_fields;
  };

  std::unordered_map<std::string, per_groups> name_to_groups;
};

namespace detail {

struct find_groups_impl {
  /// Match scheme ... { ... }.
  void operator()(ast::scheme const &node) {
    for (auto statement : node.statements)
      (*this)(statement);
  }

  /// Match scheme ... { groups ... { ... } }.
  void operator()(ast::groups const &node) {
    auto &n2g = map.name_to_groups;

    if (core::contains(n2g, node.name))
      throw std::runtime_error{
          "multiple groups with the same name are not allowed"};

    auto [it, _] = n2g.insert({node.name, {}});
    it->second.select = node.select;

    auto &uf = it->second.uniform_fields;
    auto &vf = it->second.varying_fields;

    for (auto &field_variant : node.fields) {
      std::visit(
          core::overloaded{
              [&uf, &vf](ast::n_groups::uniform_field const &field) {
                if (core::contains(vf.alias_to_name, field.alias))
                  throw std::runtime_error{"uniform field alias already used "
                                           "as varying field alias"};
                // add a uniform field to these groups
                uf.insert(field);
              },
              [&uf, &vf](ast::n_groups::varying_field const &field) {
                if (core::contains(uf.alias_to_name, field.alias))
                  throw std::runtime_error{"varying field alias already used "
                                           "as uniform field alias"};
                // add a varying field to these groups
                vf.insert(field);
              },
              [](auto const &) {
                throw std::runtime_error{"invalid ast node"};
              }},
          field_variant);
    }
  }

  /// Ignore scheme ... { global { ... } }.
  void operator()(ast::global const &) const {}

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
  groups_map map;
};

} // namespace detail

auto find_groups(ast::scheme const &scheme) {
  detail::find_groups_impl impl;
  impl(scheme);
  return std::move(impl.map);
}

} // namespace prtcl::gt
