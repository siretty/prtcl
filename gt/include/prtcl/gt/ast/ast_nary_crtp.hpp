#pragma once

#include "ast_nary_base.hpp"

#include <memory>
#include <utility>

namespace prtcl::gt::ast {

template <typename Impl_> class ast_nary_crtp : public virtual ast_nary_base {
public:
  template <typename NodeType_> class add_return {
    // {{{
  public:
    template <typename F_> auto &operator()(F_ &&f_) const {
      std::forward<F_>(f_)(std::addressof(this->_child));
      return *this;
    }

    auto *operator-> () const { return std::addressof(this->_self); }

  public:
    operator Impl_ *() const { return std::addressof(this->_self); }

  public:
    add_return(NodeType_ &child_, Impl_ &self_)
        : _child{child_}, _self{self_} {}

  private:
    NodeType_ &_child;
    Impl_ &_self;
    // }}}
  };

  template <typename NodeType_>
  add_return<NodeType_> add_child(NodeType_ &&orig_) {
    auto *node_ =
        this->base_add_child(new NodeType_{std::forward<NodeType_>(orig_)})
            ->template as_ptr<NodeType_>();
    return add_return<NodeType_>{*node_, impl()};
  }

  template <typename NodeType_>
  add_return<NodeType_> add_child(NodeType_ *orig_) {
    auto *node_ = this->base_add_child(orig_)->template as_ptr<NodeType_>();
    return add_return<NodeType_>{*node_, impl()};
  }

private:
  auto &impl() { return *this->as_ptr<Impl_>(); }
};

} // namespace prtcl::gt::ast
