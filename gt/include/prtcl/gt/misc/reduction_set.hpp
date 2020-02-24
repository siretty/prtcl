#pragma once

#include "../ast.hpp"

#include <set>

#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

struct reduce_less {
  bool
  operator()(ast::stmt::reduce const &lhs, ast::stmt::reduce const &rhs) const {
    // TODO: rework reduce_set and potentially this functor
    return lhs.field_name < rhs.field_name;
  }
};

class reduction_set {
  struct finder {
    // {{{
    void operator()(ast::stmt::reduce const &node_) { set.insert(node_); }

    void operator()(ast::stmt::foreach_neighbor const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    void operator()(ast::stmt::foreach_particle const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    void operator()(ast::stmt::procedure const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    void operator()(ast::prtcl_source_file const &node_) {
      for (auto statement : node_.statements)
        (*this)(statement);
    }

    template <typename T_> void operator()(T_) {
      // catch-all
    }

    // {{{ generic

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

    std::set<ast::stmt::reduce, reduce_less> &set;
    // }}}
  };

public:
  bool empty() const { return _set.empty(); }

public:
  auto items() const { return boost::make_iterator_range(_set); }

public:
  template <typename Node_>
  friend reduction_set make_reduction_set(Node_ const &);

private:
  std::set<ast::stmt::reduce, reduce_less> _set;
};

template <typename Node_> reduction_set make_reduction_set(Node_ const &node_) {
  reduction_set result;
  reduction_set::finder{result._set}(node_);
  return result;
}

} // namespace prtcl::gt
