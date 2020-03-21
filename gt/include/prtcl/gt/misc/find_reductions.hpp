#pragma once

#include <prtcl/core/range.hpp>

#include <prtcl/gt/ast.hpp>

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace prtcl::gt {

struct reduction_set {
public:
  bool empty() const { return alias_to_op.empty(); }

public:
  void insert(ast::n_scheme::reduce const &node) {
    auto &a2o = alias_to_op;

    // TODO: context with error_handler
    // TODO: think about how to allow reductions with different ops into the
    //       same field
    if (auto it = a2o.find(node.left_hand_side.field); it != a2o.end()) {
      if (node.op != it->second) {
        throw std::runtime_error{"multiple reductions into the same field with "
                                 "different ops are not allowed"};
      }
    }

    a2o.insert({node.left_hand_side.field, node.op});
  }

public:
  auto all() const { return core::make_iterator_range(alias_to_op); }

  auto aliases() const {
    using namespace core::range_adaptors;
    return alias_to_op | map_keys;
  }

public:
  std::unordered_map<std::string, ast::assign_op> alias_to_op;
};

struct reduction_map {
  bool empty() const {
    for (auto &[name, set] : uniform) {
      if (not set.empty())
        return true;
    }

    return global.empty();
  }

  reduction_set global;
  std::unordered_map<std::string, reduction_set> uniform;
};

namespace detail {

struct find_reductions_impl {
  void operator()(ast::n_scheme::local const &) const {}

  void operator()(ast::n_scheme::compute const &) const {}

  void operator()(ast::n_scheme::reduce const &node) {
    auto const &lhs = node.left_hand_side;

    auto &g = map.global;
    auto &u = map.uniform;

    if (lhs.index.has_value()) {
      // reducing into uniform
      if (not p_loop)
        throw std::runtime_error{
            "reduction into uniform without active foreach particle loop"};
      if (lhs.index.value() != p_loop->index)
        throw std::runtime_error{"reduction into uniform does not use active "
                                 "foreach particle index"};

      u[p_loop->group].insert(node);
    } else {
      // reducing into global
      g.insert(node);
    }
  }

  void operator()(ast::n_scheme::foreach_neighbor const &loop) {
    n_loop = &loop;

    for (auto const &statement : loop.statements)
      (*this)(statement);

    n_loop = nullptr;
  }

  void operator()(ast::n_scheme::foreach_particle const &loop) {
    p_loop = &loop;

    for (auto const &statement : loop.statements)
      (*this)(statement);

    p_loop = nullptr;
  }

  void operator()(ast::n_scheme::solve const &) {
    // solve cannot contain reductions
  }

  void operator()(ast::n_scheme::procedure const &proc) {
    for (auto const &statement : proc.statements)
      (*this)(statement);
  }

  void operator()(ast::global const &) const {}

  void operator()(ast::groups const &) const {}

  void operator()(ast::scheme const &node) {
    for (auto const &statement : node.statements)
      (*this)(statement);
  }

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
  reduction_map map;

private:
  ast::n_scheme::foreach_particle const *p_loop = nullptr;
  ast::n_scheme::foreach_neighbor const *n_loop = nullptr;
};

} // namespace detail

auto find_reductions(ast::scheme const &scheme) {
  detail::find_reductions_impl impl;
  impl(scheme);
  return std::move(impl.map);
}

auto find_reductions(ast::n_scheme::foreach_particle const &loop) {
  detail::find_reductions_impl impl;
  impl(loop);
  return std::move(impl.map);
}

} // namespace prtcl::gt
