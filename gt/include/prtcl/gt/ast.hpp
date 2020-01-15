#pragma once

#include <prtcl/core/macros.hpp>

#include <prtcl/gt/common.hpp>

#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include <iostream>

// for field_requirements
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/set_of.hpp>

#include <boost/operators.hpp>

#include <boost/range/adaptor/map.hpp>

// for reductions
#include <boost/hana/contains.hpp>
#include <boost/hana/tuple.hpp>

// for flatten_...(...)
#include <boost/container/small_vector.hpp>

#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <boost/range/iterator_range.hpp>

// for ast_tex_printer
#include <ostream>

// for ast_nary_base
#include <boost/range/adaptor/transformed.hpp>

// for ast_latex_printer
#include <boost/range/adaptor/indexed.hpp>

// for cpp_openmp_printer
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/count_if.hpp>

namespace prtcl::gt::ast {

// =====

class ast_node_base {
  // {{{
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
  // }}}
};

class ast_leaf_base : public virtual ast_node_base {};

class ast_nary_base : public virtual ast_node_base {
  // {{{
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
  // }}}
};

template <typename Impl_> class ast_nary_crtp : public virtual ast_nary_base {
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
  // }}}
};

// =====

template <typename NodeType_> auto as_ptr_range(bool throw_ = false) {
  return boost::adaptors::transformed(
      [throw_](auto *c_) { return c_->template as_ptr<NodeType_>(throw_); });
}

inline auto no_nullptr() {
  return boost::adaptors::filtered(
      [](auto *c_) -> bool { return nullptr != c_; });
}

inline auto flatten_depth_first(ast_node_base *root_) {
  class fdf_iterator : public boost::iterator_facade<
                           fdf_iterator, ast_node_base *const,
                           boost::single_pass_traversal_tag> {
    // {{{
  public:
    fdf_iterator() = default;

    explicit fdf_iterator(ast_node_base *root_) : _root{root_} {}

  private:
    friend class boost::iterator_core_access;

    void increment() {
      std::cerr << "increment" << '\n';
      // get the current node
      auto cur_node = this->dereference();
      // if the current node can have children
      if (auto cur_nary = cur_node->as_ptr<ast_nary_base>())
        // push the first child on the stack (invalid child handled later)
        _stack.push_back({cur_nary, 0});
      else if (not _stack.empty())
        // otherwise if cur_node came from the stack, move to the next child
        ++_stack.back().child_index;
      // iterate until the top of the stack refers to a valid child
      while (not _stack.back().valid()) {
        // all children of the top node have been visited, remove it
        _stack.pop_back();
        // if the stack is now empty, turn into the end iterator
        if (_stack.empty()) {
          _root = nullptr;
          return;
        }
        // increment the current top nodes child index
        ++_stack.back().child_index;
      }
    }

    bool equal(fdf_iterator const &other) const {
      std::cerr << "equal" << '\n';
      if (_root != other._root)
        return false;
      if (_stack.size() != other._stack.size())
        return false;
      for (size_t i = 0; i < _stack.size(); ++i) {
        auto const &l = _stack[i], &r = other._stack[i];
        if (l.parent != r.parent or l.child_index != r.child_index)
          return false;
      }
      return true;
    }

    ast_node_base *dereference() const {
      std::cerr << "dereference _root=" << _root->ast_node_name() << " "
                << _root << " #_stack=" << _stack.size() << " "
                << _stack.empty() << '\n';
      // return the current node
      if (_stack.empty())
        return _root;
      else
        return _stack.back().current();
    }

  private:
    struct entry {
      ast_nary_base *parent;
      size_t child_index;

      ast_node_base *current() const {
        return parent->children()[static_cast<ptrdiff_t>(child_index)];
      }

      bool valid() const { return child_index < parent->children().size(); }
    };

  private:
    ast_node_base *_root = nullptr;
    boost::container::small_vector<entry, 8> _stack = {};
    // }}}
  };

  return boost::make_iterator_range(fdf_iterator{root_}, fdf_iterator{});
}

inline std::vector<ast_node_base *> flattened(ast_node_base *root_base_) {
  std::vector<ast_node_base *> flat;
  if (auto *root = root_base_->as_ptr<ast_nary_base>()) {
    std::vector<ast_nary_base *> stack{root};
    while (not stack.empty()) {
      // pop the top of the stack
      auto *current = stack.back();
      stack.pop_back();

      // append to the flat list
      flat.emplace_back(current);

      // iterate over all children
      for (auto *node : current->children()) {
        if (auto *tree = node->as_ptr<ast_nary_base>())
          stack.push_back(tree);
        else
          flat.emplace_back(node);
      }
    }
  } else {
    flat.emplace_back(root_base_);
  }
  return flat;
}

template <typename NodeType_> auto offspring_of_type(ast_node_base *root_) {
  // {{{
  auto rng = boost::make_iterator_range(flatten_depth_first(root_));
  if (rng.begin() != rng.end())
    rng.advance_begin(1);
  return rng
         // transform into pointers to the requested node type
         | as_ptr_range<NodeType_>()
         // filter out all null pointers
         | no_nullptr();
  // }}}
}

template <typename NodeType_> bool has_offspring_of_type(ast_node_base *root_) {
  // {{{
  auto flat = flattened(root_);
  auto rng = boost::make_iterator_range(flat);
  rng.advance_begin(1);
  return 0 < boost::range::count_if(rng | no_nullptr(), [](auto *c_) -> bool {
           return c_->template as_ptr<NodeType_>();
         });
  // }}}
}

// =====

class raw_text final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view content() const { return _content; }

public:
  char const *ast_node_name() const final { return "raw_text"; }

public:
  raw_text(std::string content_) : _content{content_} {}

private:
  std::string _content;
  // }}}
};

// =====

class assignment final : public virtual ast_nary_crtp<assignment> {
  // {{{
public:
  std::string_view op() const { return _op; }

public:
  char const *ast_node_name() const final { return "assignment"; }

public:
  assignment(std::string op_) : _op{op_} {}

private:
  std::string _op;
  // }}}
};

class operation final : public virtual ast_nary_crtp<operation> {
  // {{{
public:
  std::string name() const { return _name; }

public:
  char const *ast_node_name() const final { return "operation"; }

public:
  operation(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
};

class global_field final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  auto shape() const { return boost::make_iterator_range(_shape); }

public:
  char const *ast_node_name() const final { return "global_field"; }

public:
  explicit global_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct uniform_field final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  auto shape() const { return boost::make_iterator_range(_shape); }

public:
  char const *ast_node_name() const final { return "uniform_field"; }

public:
  explicit uniform_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct varying_field final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  auto shape() const { return boost::make_iterator_range(_shape); }

public:
  char const *ast_node_name() const final { return "varying_field"; }

public:
  explicit varying_field(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct constant final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view name() const { return _name; }

  nd_dtype dtype() const { return _dtype; }

  auto shape() const { return boost::make_iterator_range(_shape); }

public:
  char const *ast_node_name() const final { return "constant"; }

public:
  explicit constant(std::string name_, nd_dtype dtype_, nd_shape shape_)
      : _name{name_}, _dtype{dtype_}, _shape{shape_} {}

private:
  std::string _name;
  nd_dtype _dtype;
  nd_shape _shape;
  // }}}
};

struct literal final : public virtual ast_leaf_base {
  // {{{
public:
  std::string_view value() const { return _value; }

  nd_dtype dtype() const { return _dtype; }

public:
  char const *ast_node_name() const final { return "literal"; }

public:
  explicit literal(std::string value_, nd_dtype dtype_)
      : _value{value_}, _dtype{dtype_} {}

private:
  std::string _value;
  nd_dtype _dtype;
  // }}}
};

class particle_subscript final
    : public virtual ast_nary_crtp<particle_subscript> {
  // {{{
public:
  char const *ast_node_name() const final { return "particle_subscript"; }
  // }}}
};

class neighbor_subscript final
    : public virtual ast_nary_crtp<neighbor_subscript> {
  // {{{
public:
  char const *ast_node_name() const final { return "neighbor_subscript"; }
  // }}}
};

class component_subscript final
    : public virtual ast_nary_crtp<component_subscript> {
  // {{{
public:
  auto index() const { return boost::make_iterator_range(_index); }

public:
  char const *ast_node_name() const final { return "component_subscript"; }

public:
  explicit component_subscript(nd_index index_) : _index{index_} {}

private:
  nd_index _index;
  // }}}
};

// =====

class collection final : public virtual ast_nary_crtp<collection> {
  // {{{
public:
  std::string_view name() const { return _name; }

public:
  char const *ast_node_name() const final { return "collection"; }

public:
  explicit collection(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
};

class procedure final : public virtual ast_nary_crtp<procedure> {
  // {{{
public:
  std::string_view name() const { return _name; }

public:
  char const *ast_node_name() const final { return "procedure"; }

public:
  explicit procedure(std::string name_) : _name{name_} {}

private:
  std::string _name;
  // }}}
};

class foreach_particle final : public virtual ast_nary_crtp<foreach_particle> {
  // {{{
public:
  char const *ast_node_name() const final { return "foreach_particle"; }
  // }}}
};

class foreach_neighbor final : public virtual ast_nary_crtp<foreach_neighbor> {
  // {{{
public:
  char const *ast_node_name() const final { return "foreach_neighbor"; }
  // }}}
};

class if_group_type final : public virtual ast_nary_crtp<if_group_type> {
  // {{{
public:
  std::string_view group_type() const { return _group_type; }

public:
  char const *ast_node_name() const final { return "if_group_type"; }

public:
  explicit if_group_type(std::string group_type_) : _group_type{group_type_} {}

private:
  std::string _group_type;
  // }}}
};

class equation final : public virtual ast_leaf_base {
  // {{{
public:
  auto *math() const { return _math.get(); }

public:
  char const *ast_node_name() const final { return "equation"; }

public:
  explicit equation(ast_node_base *math_) : _math{math_} {}

private:
  std::unique_ptr<ast_node_base> _math;
  // }}}
};

class reduction final : public virtual ast_leaf_base {
  // {{{
public:
  auto *math() const { return _math.get(); }

public:
  char const *ast_node_name() const final { return "reduction"; }

public:
  explicit reduction(ast_node_base *math_) : _math{math_} {}

private:
  std::unique_ptr<ast_node_base> _math;
  // }}}
};

// =====

struct field_requirements {
  // {{{
public:
  void add_global(std::string_view name_, nd_dtype dtype_, nd_shape shape_) {
    _global.emplace(name_, dtype_, shape_);
  }

  void add_uniform(
      std::string_view group_type_, std::string_view name_, nd_dtype dtype_,
      nd_shape shape_) {
    add_group_type(group_type_);
    _uniform.left.insert(
        {std::string{group_type_}, field_descr{name_, dtype_, shape_}});
  }

  void add_varying(
      std::string_view group_type_, std::string_view name_, nd_dtype dtype_,
      nd_shape shape_) {
    add_group_type(group_type_);
    _varying.left.insert(
        {std::string{group_type_}, field_descr{name_, dtype_, shape_}});
  }

  void add_group_type(std::string_view group_type_) {
    _group_types.emplace(group_type_);
  }

public:
  auto global_fields() const { return ::boost::make_iterator_range(_global); }

  auto uniform_fields(std::string t_) const {
    return boost::make_iterator_range(
               _uniform.left.lower_bound(t_), _uniform.left.upper_bound(t_)) |
           boost::adaptors::map_values;
  }

  auto varying_fields(std::string t_) const {

    return boost::make_iterator_range(
               _varying.left.lower_bound(t_), _varying.left.upper_bound(t_)) |
           boost::adaptors::map_values;
  }

  auto group_types() const { return boost::make_iterator_range(_group_types); }

public:
  void load(ast_node_base *node_) {
    // TODO: this implementation is inefficient (multiple traversals, ...)
    // {{{
    auto flat = flattened(node_);

    auto find_global = [this](auto &&range_) {
      for (auto gf : std::forward<decltype(range_)>(range_)) {
        add_global(gf->name(), gf->dtype(), make_nd_shape(gf->shape()));
      }
    };

    auto find_subscript = [this](auto group_type_, auto &&range_) {
      for (auto ss : std::forward<decltype(range_)>(range_)) {
        if (auto uf = ss->children()[0]->template as_ptr<uniform_field>())
          add_uniform(
              group_type_, uf->name(), uf->dtype(), make_nd_shape(uf->shape()));
        if (auto vf = ss->children()[0]->template as_ptr<varying_field>())
          add_varying(
              group_type_, vf->name(), vf->dtype(), make_nd_shape(vf->shape()));
      }
    };

    for (auto eq : flat | as_ptr_range<equation>() | no_nullptr()) {
      auto math_flat = flattened(eq->math());
      find_global(math_flat | as_ptr_range<global_field>() | no_nullptr());
    }

    for (auto rd : flat | as_ptr_range<reduction>() | no_nullptr()) {
      auto math_flat = flattened(rd->math());
      find_global(math_flat | as_ptr_range<global_field>() | no_nullptr());
    }

    for (auto fp : flat | as_ptr_range<foreach_particle>() | no_nullptr()) {
      for (auto gt : fp->children() | as_ptr_range<if_group_type>(true)) {
        add_group_type(gt->group_type());
        auto gt_flat = flattened(gt);
        for (auto eq : gt_flat | as_ptr_range<equation>() | no_nullptr()) {
          auto math_flat = flattened(eq->math());
          find_subscript(
              gt->group_type(),
              math_flat | as_ptr_range<particle_subscript>() | no_nullptr());
        }
        for (auto rd : gt_flat | as_ptr_range<reduction>() | no_nullptr()) {
          auto math_flat = flattened(rd->math());
          find_subscript(
              gt->group_type(),
              math_flat | as_ptr_range<particle_subscript>() | no_nullptr());
        }
      }
    }

    for (auto fn : flat | as_ptr_range<foreach_neighbor>() | no_nullptr()) {
      for (auto gt : fn->children() | as_ptr_range<if_group_type>(true)) {
        add_group_type(gt->group_type());
        auto gt_flat = flattened(gt);
        for (auto eq : gt_flat | as_ptr_range<equation>() | no_nullptr()) {
          auto math_flat = flattened(eq->math());
          find_subscript(
              gt->group_type(),
              math_flat | as_ptr_range<neighbor_subscript>() | no_nullptr());
        }
        for (auto rd : gt_flat | as_ptr_range<reduction>() | no_nullptr()) {
          auto math_flat = flattened(rd->math());
          find_subscript(
              gt->group_type(),
              math_flat | as_ptr_range<neighbor_subscript>() | no_nullptr());
        }
      }
    }
    // }}}
  }

private:
  struct field_descr : boost::totally_ordered<field_descr> {
    std::string name;
    nd_dtype dtype;
    nd_shape shape;

    field_descr(std::string_view name_, nd_dtype dtype_, nd_shape shape_)
        : name{name_}, dtype{dtype_}, shape{shape_} {}

    bool operator==(field_descr const &rhs_) const { return name == rhs_.name; }

    bool operator<(field_descr const &rhs_) const { return name < rhs_.name; }
  };

private:
  using group_type_fields_map = ::boost::bimaps::bimap<
      ::boost::bimaps::multiset_of<std::string>,
      ::boost::bimaps::set_of<field_descr>>;

private:
  std::set<std::string> _group_types;
  std::set<field_descr> _global;
  group_type_fields_map _uniform;
  group_type_fields_map _varying;
  // }}}
};

struct reduction_requirements {
  // {{{
public:
  void add_global(
      std::string_view op_, std::string_view name_, nd_dtype dtype_,
      nd_shape shape_) {
    _global.emplace(op_, name_, dtype_, shape_);
  }

  void add_uniform(
      std::string_view group_type_, std::string_view op_,
      std::string_view name_, nd_dtype dtype_, nd_shape shape_) {
    (void)(group_type_), (void)(op_), (void)(name_), (void)(dtype_),
        (void)(shape_);
    throw std::runtime_error{"not implemented"};
  }

public:
  auto global() const { return boost::make_iterator_range(_global); }

public:
  void load(ast_node_base *node_) {
    // TODO: handle component subscript reductees

    auto flat = flattened(node_);

    for (auto rd : flat | as_ptr_range<reduction>() | no_nullptr()) {
      auto op = rd->math()->as_ptr<operation>(true);
      auto lhs = op->children()[0];
      if (auto ps = lhs->as_ptr<particle_subscript>()) {
        auto uf = ps->children()[0]->as_ptr<uniform_field>(true);
        add_uniform(
            "", op->name(), uf->name(), uf->dtype(),
            make_nd_shape(uf->shape()));
      } else {
        auto gf = lhs->as_ptr<global_field>(true);
        add_global(
            op->name(), gf->name(), gf->dtype(), make_nd_shape(gf->shape()));
      }
    }
  }

private:
  struct reduction_descr : boost::totally_ordered<reduction_descr> {
    std::string op;
    std::string name;
    nd_dtype dtype;
    nd_shape shape;

    reduction_descr(
        std::string_view op_, std::string_view name_, nd_dtype dtype_,
        nd_shape shape_)
        : op{op_}, name{name_}, dtype{dtype_}, shape{shape_} {}

    bool operator==(reduction_descr const &rhs_) const {
      return name == rhs_.name;
    }

    bool operator<(reduction_descr const &rhs_) const {
      return name < rhs_.name;
    }
  };

private:
  std::set<reduction_descr> _global;
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
  auto unindent() {
    class unindent_result {
    public:
      unindent_result(Impl_ &impl_) : _impl{impl_} {
        _was_already_zero = (0 == _impl._indent);
        if (not _was_already_zero)
          --_impl._indent;
      }

      ~unindent_result() {
        if (not _was_already_zero)
          ++_impl._indent;
      }

    private:
      Impl_ &_impl;
      bool _was_already_zero;
    };

    return unindent_result{impl()};
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
    if (auto n = node_->as_ptr<reduction>())
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
    if (auto n = node_->as_ptr<literal>())
      return impl()(n);
    if (auto n = node_->as_ptr<particle_subscript>())
      return impl()(n);
    if (auto n = node_->as_ptr<neighbor_subscript>())
      return impl()(n);
    if (auto n = node_->as_ptr<component_subscript>())
      return impl()(n);

    if (auto n = node_->as_ptr<raw_text>())
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

class latex_printer : public ast_printer_crtp<latex_printer> {
  // {{{

  // =====
  // Algorithm and Flow Control
  // =====

public:
  void operator()(collection *node_) {
    *this << R"(\begin{PrtclCollection}{)" << node_->name() << "}" << nl;
    {
      auto indenter = this->indent();
      for (auto *child : node_->children())
        this->dispatch(child);
    }
    *this << R"(\end{PrtclCollection})" << nl;
  }

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

  void operator()(reduction *node_) {
    *this << R"(\begin{PrtclReduction})" << nl;
    {
      auto indenter = this->indent();
      this->dispatch(node_->math());
    }
    *this << nl;
    *this << R"(\end{PrtclReduction})" << nl;
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
    *this << " }_{ ";
    for (auto v : node_->index() | boost::adaptors::indexed()) {
      if (0 != v.index())
        *this << ", ";
      *this << v.value();
    }
    *this << " }";
  }

  template <typename NodeType_> void operator()(NodeType_ *) {
    throw std::runtime_error{"not implemented"};
  }

  // }}}
};

class cpp_openmp_printer : public ast_printer_crtp<cpp_openmp_printer> {
  // {{{

#define PRTCL_INDENT_RAII auto PRTCL_UNIQUE_NAME(indenter_) = this->indent()
#define PRTCL_UNINDENT_RAII                                                    \
  auto PRTCL_UNIQUE_NAME(unindenter_) = this->unindent()

  // TODO: proper configuration macros
#define PRTCL_GT_CONFIG_DEBUG_FOREACH_CHILD

  // =====
  // Algorithm and Flow Control
  // =====

public:
  void operator()(collection *node_) {
    // {{{
    field_requirements reqs;
    reqs.load(node_);

    reduction_requirements rd_reqs;
    rd_reqs.load(node_);

    *this << "#pragma once" << nl;
    *this << nl;
    *this << "#include <prtcl/rt/common.hpp>" << nl;
    *this << "#include <prtcl/rt/basic_model.hpp>" << nl;
    *this << "#include <prtcl/rt/basic_group.hpp>" << nl;
    *this << nl;
    *this << "#include <vector>" << nl;
    *this << nl;
    *this << "#include <omp.h>" << nl;
    *this << nl;
    *this << "namespace prtcl::schemes {" << nl;
    *this << nl;
    *this << "template <" << nl;
    {
      PRTCL_INDENT_RAII;
      *this << "typename ModelPolicy_" << nl;
    }
    *this << ">" << nl;
    *this << "class " << node_->name() << " {" << nl;
    {
      PRTCL_INDENT_RAII;

      cpp_access_modifier("public");
      *this << "using model_policy = ModelPolicy_;" << nl;
      *this << "using type_policy = typename model_policy::type_policy;" << nl;
      *this << "using math_policy = typename model_policy::math_policy;" << nl;
      *this << "using data_policy = typename model_policy::data_policy;" << nl;
      *this << nl;
      *this << "using nd_dtype = prtcl::rt::nd_dtype;" << nl;
      *this << nl;
      *this << "template <nd_dtype DType_> using dtype_t = typename "
               "type_policy::template dtype_t<DType_>;"
            << nl;
      *this << "template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_t = "
               "typename math_policy::template nd_dtype_t<DType_, Ns_...>;"
            << nl;
      *this << "template <nd_dtype DType_, size_t ...Ns_> using "
               "nd_dtype_data_ref_t = typename data_policy::template "
               "nd_dtype_data_ref_t<DType_, Ns_...>;"
            << nl;
      *this << nl;
      *this << "static constexpr size_t N = model_policy::dimensionality;" << nl;
      *this << nl;
      *this << "using model_type = prtcl::rt::basic_model<model_policy>;" << nl;
      *this << "using group_type = prtcl::rt::basic_group<model_policy>;" << nl;
      *this << nl;

      cpp_access_modifier("private");
      *this << "struct global_data {" << nl;
      { // {{{
        PRTCL_INDENT_RAII;
        for (auto const &gf : reqs.global_fields()) {
          *this << "nd_dtype_data_ref_t<"
                << nd_template_args(gf.dtype, gf.shape) << "> " << gf.name
                << ";" << nl;
        }

        *this << nl;

        *this << "static void _require(model_type &m_) {" << nl;
        { // {{{
          PRTCL_INDENT_RAII;
          for (auto const &gf : reqs.global_fields()) {
            *this << "m_.template add_global<"
                  << nd_template_args(gf.dtype, gf.shape) << ">(\"" << gf.name
                  << "\");" << nl;
          }
        } // }}}
        *this << "}" << nl;

        *this << nl;

        *this << "void _load(model_type const &m_) {" << nl;
        { // {{{
          PRTCL_INDENT_RAII;
          for (auto const &gf : reqs.global_fields()) {
            *this << gf.name << " = "
                  << "m_.template get_global<"
                  << nd_template_args(gf.dtype, gf.shape) << ">(\"" << gf.name
                  << "\");" << nl;
          }
        } // }}}
        *this << "}" << nl;
      } // }}}
      *this << "};" << nl;

      *this << nl;

      for (auto group_type : reqs.group_types()) {
        cpp_access_modifier("private");
        *this << "struct " << group_type << "_group_data {" << nl;
        { // {{{
          PRTCL_INDENT_RAII;

          *this << "size_t _count;" << nl;
          *this << "size_t _index;" << nl;

          *this << nl;
          *this << "// uniform fields" << nl;
          for (auto const &uf : reqs.uniform_fields(group_type)) {
            *this << "nd_dtype_data_ref_t<"
                  << nd_template_args(uf.dtype, uf.shape) << "> " << uf.name
                  << ";" << nl;
          }

          *this << nl;
          *this << "// varying fields" << nl;
          for (auto const &vf : reqs.varying_fields(group_type)) {
            *this << "nd_dtype_data_ref_t<"
                  << nd_template_args(vf.dtype, vf.shape) << "> " << vf.name
                  << ";" << nl;
          }

          *this << nl;
          *this << "static void _require(group_type &g_) {" << nl;
          { // {{{
            PRTCL_INDENT_RAII;

            *this << "// uniform fields" << nl;
            for (auto const &uf : reqs.uniform_fields(group_type)) {
              *this << "g_.template add_uniform<"
                    << nd_template_args(uf.dtype, uf.shape) << ">(\"" << uf.name
                    << "\");" << nl;
            }

            *this << nl;
            *this << "// varying fields" << nl;
            for (auto const &vf : reqs.varying_fields(group_type)) {
              *this << "g_.template add_varying<"
                    << nd_template_args(vf.dtype, vf.shape) << ">(\"" << vf.name
                    << "\");" << nl;
            }
          } // }}}
          *this << "}" << nl;

          *this << nl;
          *this << "void _load(group_type const &g_) {" << nl;
          { // {{{
            PRTCL_INDENT_RAII;

            *this << "_count = g_.size();" << nl;
            *this << nl;

            *this << "// uniform fields" << nl;
            for (auto const &uf : reqs.uniform_fields(group_type)) {
              *this << uf.name << " = "
                    << "g_.get_uniform<" << nd_template_args(uf.dtype, uf.shape)
                    << ">(\"" << uf.name << "\");" << nl;
            }

            *this << nl;
            *this << "// varying fields" << nl;
            for (auto const &vf : reqs.varying_fields(group_type)) {
              *this << vf.name << " = "
                    << "g_.get_varying<" << nd_template_args(vf.dtype, vf.shape)
                    << ">(\"" << vf.name << "\");" << nl;
            }
          } // }}}
          *this << "}" << nl;
        } // }}}
        *this << "};" << nl;
        *this << nl;
      }

      cpp_access_modifier("public");
      *this << "static void require(model_type &m_) {" << nl;
      { // {{{
        PRTCL_INDENT_RAII;
        *this << "global_data::_require(m_);" << nl;
        *this << nl;
        *this << "for (auto &group : m_.groups()) {" << nl;
        {
          for (auto const &group_type : reqs.group_types()) {
            PRTCL_INDENT_RAII;
            *this << "if (group.get_type() == \"" << group_type << "\")" << nl;
            {
              PRTCL_INDENT_RAII;
              *this << group_type << "_group_data::_require(group);" << nl;
            }
          }
        }
        *this << "}" << nl;
      } // }}}
      *this << "}" << nl;

      *this << nl;

      cpp_access_modifier("public");
      *this << "void load(model_type &m_) {" << nl;
      { // {{{
        PRTCL_INDENT_RAII;
        *this << "_group_count = m_.groups().size();" << nl;
        *this << nl;
        *this << "_data.global._load(m_);" << nl;
        *this << nl;
        *this << "auto groups = m_.groups();" << nl;
        *this << "for (size_t i = 0; i < groups.size(); ++i) {" << nl;
        {
          PRTCL_INDENT_RAII;
          *this << "auto &group = groups[static_cast<typename "
                   "decltype(groups)::difference_type>(i)];"
                << nl;
          for (auto const &group_type : reqs.group_types()) {
            *this << nl;
            *this << "if (group.get_type() == \"" << group_type << "\") {"
                  << nl;
            {
              PRTCL_INDENT_RAII;
              *this << "auto &data = _data.by_group_type." << group_type
                    << ".emplace_back();" << nl;
              *this << "data._load(group);" << nl;
              *this << "data._index = i;" << nl;
            }
            *this << "}" << nl;
          }
        }
        *this << "}" << nl;
      } // }}}
      *this << "}" << nl;

      *this << nl;

      cpp_access_modifier("private");
      *this << "struct {" << nl;
      { // {{{
        PRTCL_INDENT_RAII;
        *this << "global_data global;" << nl;
        *this << "struct {" << nl;
        {
          PRTCL_INDENT_RAII;
          for (auto group_type : reqs.group_types())
            *this << "std::vector<" << group_type << "_group_data> "
                  << group_type << ";" << nl;
        }
        *this << "} by_group_type;" << nl;
      } // }}}
      *this << "} _data;" << nl;
      *this << nl;
      *this << "struct {" << nl;
      { // {{{
        PRTCL_INDENT_RAII;
        *this << "std::vector<std::vector<std::vector<size_t>>> "
                 "neighbors;"
              << nl;
        *this << nl;
        *this << "// reductions" << nl;
        for (auto rd : rd_reqs.global()) {
          *this << "std::vector<nd_dtype_t<"
                << nd_template_args(rd.dtype, rd.shape) << ">> "
                << "rd_" << rd.name << ";" << nl;
        }
      } // }}}
      *this << "} _per_thread;" << nl;
      *this << nl;
      *this << "size_t _group_count;" << nl;

      *this << nl;

      // iterate over all child nodes
      for (auto v : node_->children() | boost::adaptors::indexed()) {
        // insert a blank line inbetween child nodes
        if (0 != v.index())
          *this << nl;

        debug_start_of_child(v.index(), v.value());

        // handle the child node
        this->dispatch(v.value());

        debug_close_of_child(v.index(), v.value());
      }
    }
    *this << "};" << nl;
    *this << nl;
    *this << "} // namespace prtcl::schemes" << nl;
    // }}}
  }

  void operator()(procedure *node_) {
    // {{{
    cpp_access_modifier("public");
    *this << "template <typename NHood_>" << nl;
    *this << "void " << node_->name() << "(NHood_ const &nhood_) {" << nl;
    {
      PRTCL_INDENT_RAII;

      *this << "// alias for the global data" << nl;
      *this << "auto &g = _data.global;" << nl;
      *this << nl;

      *this << "// alias for the math_policy member (types)" << nl;
      *this << "using o = typename math_policy::operations;" << nl;
      *this << "using c = typename math_policy::constants;" << nl;
      *this << nl;

      // iterate over all child nodes
      for (auto v : node_->children() | boost::adaptors::indexed()) {
        // insert a blank line inbetween child nodes
        if (0 != v.index())
          *this << nl;

        debug_start_of_child(v.index(), v.value());

        // handle the child node
        this->dispatch(v.value());

        debug_close_of_child(v.index(), v.value());
      }
    }
    *this << "}" << nl;
    // }}}
  }

  void operator()(foreach_particle *node_) {
    // {{{
    *this << "#pragma omp parallel" << nl << "{" << nl;
    {
      PRTCL_INDENT_RAII;

      bool const has_foreach_neighbor_offspring =
          has_offspring_of_type<foreach_neighbor>(node_);

      reduction_requirements rd_reqs;
      rd_reqs.load(node_);
      bool const has_reduction_offspring = not rd_reqs.global().empty();

      *this << "#pragma omp single" << nl << "{" << nl;
      {
        PRTCL_INDENT_RAII;
        *this << "auto const thread_count = "
                 "static_cast<size_t>(omp_get_num_threads());"
              << nl;
        if (has_foreach_neighbor_offspring) {
          *this << nl;
          *this << "_per_thread.neighbors.resize(thread_count);" << nl;
        }
        if (has_reduction_offspring) {
          *this << nl;
          for (auto rd : rd_reqs.global()) {
            *this << "_per_thread.rd_" << rd.name << ".resize(thread_count);"
                  << nl;
          }
        }
      }
      *this << "} // pragma omp single" << nl;

      *this << nl;
      *this << "auto const thread_index = "
               "static_cast<size_t>(omp_get_thread_num());"
            << nl;
      *this << nl;

      if (has_foreach_neighbor_offspring) {
        *this << "// select and resize the neighbor storage for the current "
                 "thread"
              << nl;
        *this << "auto &neighbors = _per_thread.neighbors[thread_index];" << nl;
        *this << "neighbors.resize(_group_count);" << nl;
        *this << nl;
        *this << "for (auto &pgn : neighbors)" << nl;
        {
          PRTCL_INDENT_RAII;
          *this << "pgn.reserve(100);" << nl;
        }
        *this << nl;
      }

      if (has_reduction_offspring) {
        *this << "// select the per-thread reduction variables" << nl;
        for (auto rd : rd_reqs.global()) {
          *this << "auto &rd_" << rd.name << " = _per_thread.rd_" << rd.name
                << "[thread_index];" << nl;
        }
        *this << nl;
        *this << "// initialize the per-thread reduction variables" << nl;
        for (auto rd : rd_reqs.global()) {
          *this << "rd_" << rd.name << " = "
                << rd_initializer(rd.op, rd.dtype, rd.shape) << ";" << nl;
        }
        *this << nl;
      }

      // iterate over all child nodes
      for (auto v : node_->children() | boost::adaptors::indexed()) {
        // insert a blank line inbetween child nodes
        if (0 != v.index())
          *this << nl;

        debug_start_of_child(v.index(), v.value());

        // handle the child node
        this->dispatch(v.value());

        debug_close_of_child(v.index(), v.value());
      }

      if (has_reduction_offspring) {
        *this << nl;
        *this << "// combine global reductions" << nl;
        *this << "#pragma omp critical" << nl;
        *this << "{" << nl;
        for (auto rd : rd_reqs.global()) {
          PRTCL_INDENT_RAII;
          auto field_access = "g." + rd.name + "[0]";
          *this << field_access << " = ";
          using boost::hana::in, boost::hana::make_tuple;
          if (rd.op ^ in ^ make_tuple("+", "-", "*", "/"))
            *this << "( " << field_access << " ) " << rd.op << " ( rd_"
                  << rd.name << " )";
          else
            *this << "o::" << rd.op << "( " << field_access << ", rd_"
                  << rd.name << " )";
          *this << " ;" << nl;
        }
        *this << "} // pragma omp critical" << nl;
      }
    }

    *this << "} // pragma omp parallel" << nl;
    // }}}
  }

  void operator()(foreach_neighbor *node_) {
    // {{{
    *this << "if (!has_neighbors) {" << nl;
    {
      PRTCL_INDENT_RAII;
      *this << "nhood_.neighbors(p._index, i, [&neighbors](auto n_index, "
               "auto j) {"
            << nl;
      {
        PRTCL_INDENT_RAII;
        *this << "neighbors[n_index].push_back(j);" << nl;
      }
      *this << "});" << nl;
      *this << "has_neighbors = true;" << nl;
    }
    *this << "}" << nl;
    *this << nl;

    // iterate over all child nodes
    for (auto v : node_->children() | boost::adaptors::indexed()) {
      // insert a blank line inbetween child nodes
      if (0 != v.index())
        *this << nl;

      debug_start_of_child(v.index(), v.value());

      // handle the child node
      this->dispatch(v.value());

      debug_close_of_child(v.index(), v.value());
    }
    // }}}
  }

  void operator()(if_group_type *node_) {
    // {{{
    std::string_view group_name;
    std::string_view index_name;
    std::string_view index_bound;

    if (not _cur_p) {
      _cur_p = node_->group_type();
      group_name = "p";
      index_name = "i";
      index_bound = "p._count";
    } else if (not _cur_n) {
      _cur_n = node_->group_type();
      group_name = "n";
      index_name = "j";
      index_bound = "neighbors[n._index].size()";

    } else
      throw std::runtime_error("invalid loop nesting");

    *this << "for (auto &" << group_name << " : _data.by_group_type."
          << node_->group_type() << ") {" << nl;
    {
      PRTCL_INDENT_RAII;

      if (_cur_p and not _cur_n)
        *this << "#pragma omp for" << nl;

      *this << "for (size_t " << index_name << " = 0; " << index_name << " < "
            << index_bound << "; ++" << index_name << ") {" << nl;
      {
        PRTCL_INDENT_RAII;

        if (_cur_p and not _cur_n and
            has_offspring_of_type<foreach_neighbor>(node_)) {
          *this << "for (auto &pgn : neighbors)" << nl;
          {
            PRTCL_INDENT_RAII;
            *this << "pgn.clear();" << nl;
          }
          *this << nl;
          *this << "bool has_neighbors = false;" << nl;
          *this << nl;
        }

        // iterate over all child nodes
        for (auto v : node_->children() | boost::adaptors::indexed()) {
          // insert a blank line inbetween child nodes
          if (0 != v.index())
            *this << nl;

          debug_start_of_child(v.index(), v.value());

          // handle the child node
          this->dispatch(v.value());

          debug_close_of_child(v.index(), v.value());
        }
      }
      *this << "}" << nl;
    }
    *this << "}" << nl;

    if (_cur_n) {
      _cur_n.reset();
    } else if (_cur_p) {
      _cur_p.reset();
    } else
      throw std::runtime_error("invalid loop nesting");
    // }}}
  }

  void operator()(equation *node_) {
    this->dispatch(node_->math());
    *this << " ;" << nl;
  }

  void operator()(reduction *node_) {
    // {{{
    auto op = node_->math()->as_ptr<operation>(true);

    std::string rd_name = "rd_";
    {
      auto lhs = op->children()[0];
      if (auto ps = lhs->as_ptr<particle_subscript>()) {
        auto uf = ps->children()[0]->as_ptr<uniform_field>(true);
        rd_name += uf->name();
      } else {
        auto gf = lhs->as_ptr<global_field>(true);
        rd_name += gf->name();
      }
    }
    *this << rd_name << " = ";

    // temporarily replace the lhs node of the reduction for code generation
    raw_text tmp_lhs{rd_name};
    auto old_lhs = op->base_replace_child(0, &tmp_lhs);
    this->dispatch(op);
    op->base_replace_child(0, old_lhs);

    *this << " ;" << nl;
    // }}}
  }

  // =====
  // Mathematics
  // =====

  void operator()(assignment *node_) {
    // {{{
    if (2 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    this->dispatch(node_->children()[0]);
    *this << ' ';

    auto op = node_->op();
    if ("=" == op)
      *this << op;
    else if (2 == op.size() and '=' == op[1])
      *this << op;
    else
      throw std::runtime_error{"invalid assignment op"};

    *this << ' ';
    this->dispatch(node_->children()[1]);
    // }}}
  }

  void operator()(operation *node_) {
    // {{{
    static std::set<std::string_view> const uops = {"-"};
    static std::set<std::string_view> const bops = {"+", "-", "*", "/"};

    auto const arity = node_->children().size();
    auto const name = node_->name();

    if (1 == arity and uops.find(name) != uops.end()) {
      *this << name << "( ";
      this->dispatch(node_->children()[0]);
      *this << " )";
    } else if (2 == arity and bops.find(name) != bops.end()) {
      *this << "( ";
      this->dispatch(node_->children()[0]);
      *this << " ) " << name << " ( ";
      this->dispatch(node_->children()[1]);
      *this << " )";
    } else {
      *this << "o::" << name << "( ";
      for (auto v : node_->children() | boost::adaptors::indexed()) {
        if (0 != v.index())
          *this << ", ";

        this->dispatch(v.value());
      }
      *this << " )";
    }
    // }}}
  }

public:
  void operator()(global_field *node_) {
    // global field data has only one value
    *this << "g." << node_->name() << "[0]";
  }

  void operator()(uniform_field *node_) {
    // uniform field data has only one value
    *this << node_->name() << "[0]";
  }

  void operator()(varying_field *node_) { *this << node_->name(); }

  void operator()(constant *node_) {
    // {{{
    if ("particle_count" == node_->name())
      *this << "p._count";
    else if ("neighbor_count" == node_->name())
      *this << "n._count";
    else
      *this << "c::template " << node_->name() << "<"
            << nd_template_args(node_->dtype(), node_->shape()) << ">()";
    // }}}
  }

  void operator()(literal *node_) {
    // {{{
    *this << "static_cast<dtype_t<" << dtype_to_string(node_->dtype()) << ">>("
          << node_->value() << ")";
    // }}}
  }

  void operator()(particle_subscript *node_) {
    // {{{
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    // access particle data
    *this << "p.";
    // handle the child node
    this->dispatch(node_->children()[0]);
    // index varying fields with the particle-index
    if (node_->children()[0]->as_ptr<ast::varying_field>())
      *this << "[i]";
    // }}}
  }

  void operator()(neighbor_subscript *node_) {
    // {{{
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    // access neighbor data
    *this << "n.";
    // handle the child node
    this->dispatch(node_->children()[0]);
    // index varying fields with the neighbor-index
    if (node_->children()[0]->as_ptr<ast::varying_field>())
      *this << "[j]";
    // }}}
  }

  void operator()(component_subscript *node_) {
    // {{{
    if (1 != node_->children().size())
      throw std::runtime_error{"invalid number of children"};

    *this << "( ";
    this->dispatch(node_->children()[0]);
    *this << " )[ ";
    for (auto v : node_->index() | boost::adaptors::indexed()) {
      if (0 != v.index())
        *this << ", ";
      *this << v.value();
    }
    *this << " ]";
    // }}}
  }

  void operator()(raw_text *node_) { *this << node_->content(); }

  template <typename NodeType_> void operator()(NodeType_ *) {
    throw std::runtime_error{"not implemented"};
  }

  // =====
  // C++ Code Generation Helper Functions
  // =====

private:
  void cpp_access_modifier(std::string_view name_) {
    PRTCL_UNINDENT_RAII;
    *this << name_ << ":" << nl;
  }

private:
  static std::string dtype_to_string(nd_dtype dtype_) {
    return "nd_dtype::" + to_string(dtype_);
  }

  static std::string nd_data_ref_type(nd_dtype dtype_) {
    return "nd_" + to_string(dtype_) + "_data_ref_t";
  }

  static std::string nd_type(nd_dtype dtype_) {
    return "nd_" + to_string(dtype_) + "_t";
  }

  template <typename Shape_>
  static std::string nd_template_args(nd_dtype dtype_, Shape_ &&shape_) {
    auto shape = make_nd_shape(std::forward<Shape_>(shape_));
    return dtype_to_string(dtype_) + (shape.size() ? ", " : "") +
           shape_to_string(shape);
  }

  template <typename Shape_>
  static std::string
  rd_initializer(std::string_view op_, nd_dtype dtype_, Shape_ &&shape_) {
    using boost::hana::in, boost::hana::make_tuple;
    auto nd_args = nd_template_args(dtype_, std::forward<Shape_>(shape_));
    if (op_ ^ in ^ make_tuple("+", "-"))
      return "c::template zeros<" + nd_args + ">()";
    if (op_ ^ in ^ make_tuple("*", "/"))
      return "c::template ones<" + nd_args + ">()";
    if (op_ == "max")
      return "c::template negative_infinity<" + nd_args + ">()";
    if (op_ == "min")
      return "c::template positive_infinity<" + nd_args + ">()";
    else
      throw std::runtime_error("invalid reduction op");
  }

private:
  template <typename Shape_>
  static std::string shape_to_string(Shape_ &&shape_) {
    std::ostringstream ss;
    for (auto v : std::forward<Shape_>(shape_) | boost::adaptors::indexed()) {
      if (0 != v.index())
        ss << ", ";

      if (auto extent = v.value(); 0 == extent)
        ss << "N";
      else
        ss << extent;
    }
    return ss.str();
  }

  // =====
  // Debugging Helper Functions
  // =====

private:
  template <typename Index_>
  void debug_start_of_child(Index_ index_, ast_node_base *child_) {
#ifdef PRTCL_GT_CONFIG_DEBUG_FOREACH_CHILD
    *this << "// start of child #" << index_ << " " << child_->ast_node_name()
          << nl;
#endif
  }

private:
  template <typename Index_>
  void debug_close_of_child(Index_ index_, ast_node_base *child_) {
#ifdef PRTCL_GT_CONFIG_DEBUG_FOREACH_CHILD
    *this << "// close of child #" << index_ << " " << child_->ast_node_name()
          << nl;
#endif
  }

  // }}}

public:
  using ast_printer_crtp<cpp_openmp_printer>::ast_printer_crtp;

private:
  std::optional<std::string> _cur_p = std::nullopt;
  std::optional<std::string> _cur_n = std::nullopt;
};

// =====

} // namespace prtcl::gt::ast
