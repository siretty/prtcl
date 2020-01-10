#pragma once

#include <prtcl/gt/nd_dtype.hpp>
#include <prtcl/gt/nd_index.hpp>
#include <prtcl/gt/nd_shape.hpp>

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

// for ast_tex_printer
#include <ostream>

// for ast_nary_base
#include <boost/range/adaptor/transformed.hpp>

// for ast_latex_printer
#include <boost/range/adaptor/indexed.hpp>

namespace prtcl::gt::ast {

// =====

class ast_node_base {
  // {{{
public:
  virtual ~ast_node_base() {}

public:
  template <typename NodeType_> auto &as_ref() {
    // dispatch to the const-version
    return const_cast<NodeType_ &>(std::as_const(*this).as_ref<NodeType_>());
  }

  template <typename NodeType_> auto &as_ref() const {
    return dynamic_cast<NodeType_ const &>(*this);
  }

  template <typename NodeType_> auto *as_ptr(bool throw_ = false) {
    // dispatch to the const-version
    return const_cast<NodeType_ *>(
        std::as_const(*this).as_ptr<NodeType_>(throw_));
  }

  template <typename NodeType_> auto *as_ptr(bool throw_ = false) const {
    if (throw_)
      return std::addressof(this->as_ref<NodeType_>());
    else
      return dynamic_cast<NodeType_ const *>(this);
  }

public:
  ast_node_base() = default;

  ast_node_base(ast_node_base const &) = delete;
  ast_node_base &operator=(ast_node_base const &) = delete;

  ast_node_base(ast_node_base &&) = default;
  ast_node_base &operator=(ast_node_base &&) = default;
  // }}}
};

class ast_leaf_base : public ast_node_base {};

class ast_nary_base : public ast_node_base {
  // {{{
public:
  ast_node_base *base_add_child(ast_node_base *node_) {
    return _children.emplace_back(node_).get();
  }

public:
  auto children() const {
    return _children |
           boost::adaptors::transformed([](auto &p_) { return p_.get(); });
  }

private:
  std::vector<std::unique_ptr<ast_node_base>> _children;
  // }}}
};

template <typename Impl_> class ast_nary_crtp : public ast_nary_base {
  // {{{
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
    auto *node_ = static_cast<NodeType_ *>(
        this->base_add_child(new NodeType_{std::forward<NodeType_>(orig_)}));
    return add_return<NodeType_>{*node_, impl()};
  }

  template <typename NodeType_>
  add_return<NodeType_> add_child(NodeType_ *orig_) {
    auto *node_ = static_cast<NodeType_ *>(this->base_add_child(orig_));
    return add_return<NodeType_>{*node_, impl()};
  }

private:
  auto &impl() { return *static_cast<Impl_ *>(this); }
  // }}}
};

// =====

class assignment : public ast_nary_crtp<assignment> {
  // {{{
public:
  std::string_view op() const { return _op; }

public:
  assignment(std::string op_) : _op{op_} {}

private:
  std::string _op;
  // }}}
};

class operation : public ast_nary_crtp<operation> {
  // {{{
public:
  std::string name() const { return _name; }

public:
  operation(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
};

class global_field : public ast_leaf_base {
  // {{{
public:
  std::string name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  nd_shape shape() const { return _shape; }

public:
  explicit global_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct uniform_field : ast_leaf_base {
  // {{{
public:
  std::string name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  nd_shape shape() const { return _shape; }

public:
  explicit uniform_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct varying_field : ast_leaf_base {
  // {{{
public:
  std::string name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  nd_shape shape() const { return _shape; }

public:
  explicit varying_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct constant : ast_leaf_base {
  // {{{
public:
  std::string name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  nd_shape shape() const { return _shape; }

public:
  explicit constant(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

class particle_subscript : public ast_nary_crtp<particle_subscript> {};

class neighbor_subscript : public ast_nary_crtp<neighbor_subscript> {};

class component_subscript : public ast_nary_crtp<component_subscript> {
  // {{{
public:
  nd_index index() const { return _index; }

public:
  explicit component_subscript(nd_index index_) : _index{index_} {}

private:
  nd_index _index;
  // }}}
};

// =====

class procedure : public ast_nary_crtp<procedure> {
  // {{{
public:
  std::string name() const { return _name; }

public:
  explicit procedure(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
};

class foreach_particle : public ast_nary_crtp<foreach_particle> {};

class foreach_neighbor : public ast_nary_crtp<foreach_neighbor> {};

class if_group_type : public ast_nary_crtp<if_group_type> {
  // {{{
public:
  std::string group_type() const { return _group_type; }

public:
  explicit if_group_type(std::string group_type_) : _group_type{group_type_} {}

private:
  std::string _group_type;
  // }}}
};

class equation : public ast_leaf_base {
  // {{{
public:
  auto *math() const { return _math.get(); }

public:
  explicit equation(ast_node_base *math_) : _math{math_} {}

private:
  std::unique_ptr<ast_node_base> _math;
  // }}}
};

// =====

template <typename Impl_> class ast_printer_crtp {
  // {{{
protected:
  enum nl_type { nl };

protected:
  auto indent() {
    class indent_result {
    public:
      indent_result(Impl_ &impl_) : _impl{impl_} { ++_impl._indent; }

      ~indent_result() { --_impl._indent; }

    private:
      Impl_ &_impl;
    };

    return indent_result{impl()};
  }

protected:
  auto &operator<<(nl_type) {
    _out << '\n';
    _indent_next = true;
    return this->impl();
  }

  template <typename Arg_> auto &operator<<(Arg_ &&arg_) {
    if (_indent_next) {
      for (size_t i = 0; i < 2 * _indent; ++i)
        _out << ' ';
      _indent_next = false;
    }
    _out << std::forward<Arg_>(arg_);
    return this->impl();
  }

public:
  void dispatch(ast_node_base *node_) {
    if (auto n = node_->as_ptr<procedure>())
      return impl()(n);
    if (auto n = node_->as_ptr<foreach_particle>())
      return impl()(n);
    if (auto n = node_->as_ptr<foreach_neighbor>())
      return impl()(n);
    if (auto n = node_->as_ptr<if_group_type>())
      return impl()(n);
    if (auto n = node_->as_ptr<equation>())
      return impl()(n);

    if (auto n = node_->as_ptr<assignment>())
      return impl()(n);
    if (auto n = node_->as_ptr<operation>())
      return impl()(n);
    if (auto n = node_->as_ptr<global_field>())
      return impl()(n);
    if (auto n = node_->as_ptr<uniform_field>())
      return impl()(n);
    if (auto n = node_->as_ptr<varying_field>())
      return impl()(n);
    if (auto n = node_->as_ptr<constant>())
      return impl()(n);
    if (auto n = node_->as_ptr<particle_subscript>())
      return impl()(n);
    if (auto n = node_->as_ptr<neighbor_subscript>())
      return impl()(n);
    if (auto n = node_->as_ptr<component_subscript>())
      return impl()(n);

    throw std::runtime_error("not implemented");
  }

private:
  Impl_ &impl() { return *static_cast<Impl_ *>(this); }

public:
  ast_printer_crtp(std::ostream &out_) : _out{out_} {}

private:
  std::ostream &_out;
  size_t _indent = 0;
  bool _indent_next = true;
  // }}}
};

class ast_latex_printer : public ast_printer_crtp<ast_latex_printer> {
  // {{{

  // =====
  // Algorithm and Flow Control
  // =====

public:
  void operator()(procedure *node_) {
    *this << R"(\begin{PrtclProcedure}{)" << node_->name() << "}" << nl;
    {
      auto indenter = this->indent();
      for (auto *child : node_->children())
        this->dispatch(child);
    }
    *this << R"(\end{PrtclProcedure})" << nl;
  }

  void operator()(foreach_particle *node_) {
    *this << R"(\begin{PrtclForeachParticle})" << nl;
    {
      auto indenter = this->indent();
      for (auto *child : node_->children())
        this->dispatch(child);
    }
    *this << R"(\end{PrtclForeach})" << nl;
  }

  void operator()(foreach_neighbor *node_) {
    *this << R"(\begin{PrtclForeachNeighbor})" << nl;
    {
      auto indenter = this->indent();
      for (auto *child : node_->children())
        this->dispatch(child);
    }
    *this << R"(\end{PrtclForeach})" << nl;
  }

  void operator()(if_group_type *node_) {
    *this << R"(\begin{PrtclIfGroupType}{)" << node_->group_type() << "}" << nl;
    {
      auto indenter = this->indent();
      for (auto *child : node_->children())
        this->dispatch(child);
    }
    *this << R"(\end{PrtclIfGroupType})" << nl;
  }

  void operator()(equation *node_) {
    *this << R"(\begin{PrtclEquation})" << nl;
    {
      auto indenter = this->indent();
      this->dispatch(node_->math());
    }
    *this << nl;
    *this << R"(\end{PrtclEquation})" << nl;
  }

  // =====
  // Mathematics
  // =====

  void operator()(assignment *node_) {
    if (2 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    this->dispatch(node_->children()[0]);
    *this << ' ';

    auto op = node_->op();
    if ("=" == op)
      *this << op;
    else if (2 == op.size() and '=' == op[1])
      *this << R"(\,\mathop{)" << op[0] << R"(\mkern-5mu)" << op[1] << R"(}\,)";
    else
      throw std::runtime_error{"invalid assignment op"};

    *this << ' ';
    this->dispatch(node_->children()[1]);
  }

  void operator()(operation *node_) {
    static std::set<std::string_view> const uops = {"-", "norm",
                                                    "norm_squared"};
    static std::set<std::string_view> const bops = {"+", "-", "*", "/", "dot"};

    auto const arity = node_->children().size();
    auto const name = node_->name();

    if (1 == arity and uops.find(name) != uops.end()) {
      *this << name << R"(\left( )";
      this->dispatch(node_->children()[0]);
      *this << R"( \right))";
    } else if (2 == arity and bops.find(name) != bops.end()) {
      this->dispatch(node_->children()[0]);
      *this << ' ' << name << ' ';
      this->dispatch(node_->children()[1]);
    } else {
      *this << name << R"(\left( )";
      for (auto v : node_->children() | boost::adaptors::indexed()) {
        if (0 != v.index())
          *this << ", ";
        this->dispatch(v.value());
      }
      *this << R"( \right))";
    }
  }

private:
  template <typename Node_> void print_field_suffix(Node_ *node_) {
    // {{{
    switch (node_->dtype()) {
    case nd_dtype::real: {
      *this << "Real";
    } break;
    case nd_dtype::integer: {
      *this << "Integer";
    } break;
    case nd_dtype::boolean: {
      *this << "Boolean";
    } break;
    default:
      throw std::runtime_error{"invalid nd_dtype"};
    }

    switch (node_->shape().size()) {
    case 0: {
      *this << "Scalar";
    } break;
    case 1: {
      *this << "Vector";
    } break;
    case 2: {
      *this << "Matrix";
    } break;
    default: {
      *this << "Block";
    } break;
    }

    *this << "{" << node_->name() << "}";
    // }}}
  }

public:
  void operator()(global_field *node_) {
    *this << R"(\PrtclGlobalField)";
    print_field_suffix(node_);
  }

  void operator()(uniform_field *node_) {
    *this << R"(\PrtclUniformField)";
    print_field_suffix(node_);
  }

  void operator()(varying_field *node_) {
    *this << R"(\PrtclVaryingField)";
    print_field_suffix(node_);
  }

  void operator()(constant *node_) {
    *this << R"(\PrtclConstant)";
    print_field_suffix(node_);
  }

  void operator()(particle_subscript *node_) {
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    *this << "{ ";
    this->dispatch(node_->children()[0]);
    *this << " }_i";
  }

  void operator()(neighbor_subscript *node_) {
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    *this << "{ ";
    this->dispatch(node_->children()[0]);
    *this << " }_j";
  }

  void operator()(component_subscript *node_) {
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    *this << "{ ";
    this->dispatch(node_->children()[0]);
    *this << " }_{";
    for (auto v : node_->index() | boost::adaptors::indexed()) {
      if (0 != v.index())
        *this << ", ";
      *this << v.value();
    }
    *this << "}";
  }

  template <typename NodeType_> void operator()(NodeType_ *) {
    throw std::runtime_error{"not implemented"};
  }

  // }}}
};

// =====

} // namespace prtcl::gt::ast
