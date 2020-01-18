#pragma once

#include "ast_node_base.hpp"

#include <memory>
#include <vector>

#include <boost/range/adaptor/transformed.hpp>

namespace prtcl::gt::ast {

class ast_nary_base : public virtual ast_node_base {
public:
  ast_node_base *base_add_child(ast_node_base *node_) {
    return _children.emplace_back(node_).get();
  }

  ast_node_base *base_replace_child(size_t index_, ast_node_base *new_child_) {
    auto old_child = _children[index_].release();
    _children[index_] = std::unique_ptr<ast_node_base>(new_child_);
    return old_child;
  }

public:
  auto children() const {
    return _children |
           boost::adaptors::transformed([](auto &p_) { return p_.get(); });
  }

  auto release_children() {
    return _children |
           boost::adaptors::transformed([](auto &p_) { return p_.release(); });
  }

private:
  std::vector<std::unique_ptr<ast_node_base>> _children;
};

} // namespace prtcl::gt
