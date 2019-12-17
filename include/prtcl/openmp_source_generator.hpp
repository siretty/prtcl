#pragma once

#include <prtcl/expr.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/is_any_of.hpp>
#include <prtcl/meta/overload.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/type.hpp>

#include <algorithm>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <boost/lexical_cast.hpp>

namespace prtcl {

namespace n_openmp {

// class requirements {{{

struct requirements {
  using scheme_field =
      std::variant<std::monostate,
                   ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
                   ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
                   ::prtcl::expr::field<tag::kind::global, tag::type::matrix>>;
  using group_field =
      std::variant<std::monostate,
                   ::prtcl::expr::field<tag::kind::uniform, tag::type::scalar>,
                   ::prtcl::expr::field<tag::kind::uniform, tag::type::vector>,
                   ::prtcl::expr::field<tag::kind::uniform, tag::type::matrix>,
                   ::prtcl::expr::field<tag::kind::varying, tag::type::scalar>,
                   ::prtcl::expr::field<tag::kind::varying, tag::type::vector>,
                   ::prtcl::expr::field<tag::kind::varying, tag::type::matrix>>;

  std::vector<scheme_field> scheme_fields;
  std::unordered_map<std::string, std::vector<group_field>> group_fields;
};

// }}}

// class extract_requirements_xform {{{

class extract_requirements_xform : ::prtcl::expr::xform_helper {
public:
  /// Record a global field.
  template <typename TT>
  auto operator()(term<::prtcl::expr::field<tag::kind::global, TT>> term_) {
    _reqs.scheme_fields.emplace_back(term_.value());
    return term_;
  }

  /// Record a field of the active group.
  template <typename KT, typename TT>
  auto
  operator()(subs<term<::prtcl::expr::field<KT, TT>>, term<tag::group::active>>
                 expr_) {
    // throws if _cur_p is not set
    auto [it, inserted] = _reqs.group_fields.insert({_cur_p.value(), {}});
    it->second.emplace_back(expr_.left().value());
    return expr_;
  }

  /// Record a field of the passive group.
  template <typename KT, typename TT>
  auto
  operator()(subs<term<::prtcl::expr::field<KT, TT>>, term<tag::group::passive>>
                 expr_) {
    // throws if _cur_n is not set
    auto [it, inserted] = _reqs.group_fields.insert({_cur_n.value(), {}});
    it->second.emplace_back(expr_.left().value());
    return expr_;
  }

  /// Record fields in equations.
  template <typename E> auto operator()(term<::prtcl::expr::eq<E>> expr_) {
    boost::yap::transform(expr_.value().expression, *this);
    return expr_;
  }

  /// Record fields in reductions.
  template <typename RT, typename LHS, typename RHS>
  auto operator()(term<::prtcl::expr::rd<RT, LHS, RHS>> expr_) {
    boost::yap::transform(boost::yap::make_terminal(expr_.value().lhs), *this);
    boost::yap::transform(expr_.value().rhs, *this);
    return expr_;
  }

  /// Fill in the appropriate group selector.
  template <typename... Exprs>
  auto operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    auto &selector = expr.elements[0_c].value();
    // fill in the current group types
    if (!_cur_p && !_cur_n)
      _cur_p = selector.type;
    else if (_cur_p && !_cur_n)
      _cur_n = selector.type;
    else
      throw "invalid selector";
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform(std::forward<decltype(e)>(e), *this);
        });
    // clear the current group types
    if (_cur_p && _cur_n)
      _cur_n.reset();
    else if (_cur_p && !_cur_n)
      _cur_p.reset();
    else
      throw "invalid selector";
    return expr;
  }

public:
  extract_requirements_xform(requirements &reqs_) : _reqs{reqs_} {}

private:
  requirements &_reqs;
  std::optional<std::string> _cur_p, _cur_n;
};

// }}}

// extract_requirements(...) {{{

template <typename Expr>
static void extract_requirements(requirements &reqs_, Expr &&expr_) {
  boost::yap::transform(std::forward<Expr>(expr_),
                        extract_requirements_xform{reqs_});

  // uniquify(...) {{{
  auto uniquify = [](auto &fields_) {
    std::sort(fields_.begin(), fields_.end(), [](auto lhs_var_, auto rhs_var_) {
      return std::visit(
          [](auto lhs_, auto rhs_) -> bool {
            if constexpr (meta::is_any_of_v<
                              meta::remove_cvref_t<decltype(lhs_)>,
                              std::monostate> or
                          meta::is_any_of_v<
                              meta::remove_cvref_t<decltype(rhs_)>,
                              std::monostate>)
              throw "empty variant";
            else {
              auto tuplify = [](auto f_) {
                return std::make_tuple(
                    boost::lexical_cast<std::string>(f_.kind_tag),
                    boost::lexical_cast<std::string>(f_.kind_tag), f_.value);
              };
              return tuplify(lhs_) < tuplify(rhs_);
            }
          },
          lhs_var_, rhs_var_);
    });
    auto last = std::unique(fields_.begin(), fields_.end());
    fields_.erase(last, fields_.end());
  };
  // }}}

  uniquify(reqs_.scheme_fields);
  for (auto &pair : reqs_.group_fields)
    uniquify(pair.second);
}

// }}}

// class cxx_printer {{{

class cxx_printer {
  std::ostream &_stream;
  size_t _indent;

private:
  cxx_printer(std::ostream &stream_, size_t indent_)
      : _stream{stream_}, _indent{indent_} {}

public:
  cxx_printer(std::ostream &stream_) : cxx_printer(stream_, 0) {}

public:
  auto &indent() const {
    for (size_t i = 0; i < _indent * 2; ++i)
      _stream.put(' ');
    return _stream;
  }

  auto &stream() const { return _stream; }

public:
  template <typename RHS> decltype(auto) operator<<(RHS &&rhs) const {
    return _stream << std::forward<RHS>(rhs);
  }

public:
  void increase_indent() { ++_indent; }

  void decrease_indent() { --_indent; }

  auto indented() const {
    return cxx_printer{
        _stream,
        _indent + 1,
    };
  }
};

// }}}

// class math_leaf_xform {{{

class math_leaf_xform : protected ::prtcl::expr::xform_helper {
public:
  /// Generate access to global field.
  template <typename TT>
  void
  operator()(term<::prtcl::expr::field<tag::kind::global, TT>> term_) const {
    _printer << "g." << term_.value().value << "[0]";
  }

  /// Generate access to uniform field of the active group.
  template <typename TT>
  void operator()(subs<term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                       term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[0]";
  }

  /// Generate access to uniform field of the passive group.
  template <typename TT>
  void operator()(subs<term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                       term<tag::group::passive>>
                      expr_) const {
    _printer << "n." << expr_.left().value().value << "[0]";
  }

  /// Generate access to varying field of the active group.
  template <typename TT>
  void operator()(subs<term<::prtcl::expr::field<tag::kind::varying, TT>>,
                       term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[i]";
  }

  /// Generate access to varying field of the passive group.
  template <typename TT>
  void operator()(subs<term<::prtcl::expr::field<tag::kind::varying, TT>>,
                       term<tag::group::passive>>
                      expr_) const {
    _printer << "n." << expr_.left().value().value << "[j]";
  }

public:
  math_leaf_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

// class math_expr_xform {{{

class math_expr_xform : ::prtcl::expr::xform_helper {
public:
  /// Generate constant.
  template <typename T> void operator()(term<T> const &term_) const {
    _printer << term_.value();
  }

  /// Generate code for calling some function.
  template <typename CT, typename... Args>
  std::enable_if_t<tag::is_call_v<CT>, void>
  operator()(call_expr<term<CT>, Args...> expr_) const {
    using namespace boost::hana::literals;
    // begin the function call
    _printer << expr_.elements[0_c].value() << "(";
    // transforms a single argument of the call
    auto xform_arg = [this](bool leading_comma, auto &&e) {
      if (leading_comma)
        _printer << ", ";
      boost::yap::transform_strict(std::forward<decltype(e)>(e),
                                   math_leaf_xform{_printer},
                                   math_expr_xform{_printer});
    };
    // the first argument has no leading comma
    if constexpr (1 < sizeof...(Args))
      xform_arg(false, expr_.elements[1_c]);
    // all but the first argument have leading commas
    boost::hana::for_each(
        boost::hana::slice_c<2, sizeof...(Args) + 1>(expr_.elements),
        [&xform_arg](auto &&e) {
          xform_arg(true, std::forward<decltype(e)>(e));
        });
    // close the function call
    _printer << ")";
  }

  /// Generate unary operator.
  template <expr_kind K, typename Expr,
            typename = std::enable_if_t<is_uop_v<K>>>
  void operator()(expr<K, Expr> const &expr_) const {
    using namespace boost::hana::literals;
    _printer << "( " << boost::yap::op_string(K);
    boost::yap::transform_strict(expr_.elements[0_c], math_leaf_xform{_printer},
                                 math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate binary operator.
  template <expr_kind K, typename LHS, typename RHS,
            typename = std::enable_if_t<is_bop_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    _printer << "( ";
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer},
                                 math_expr_xform{_printer});
    _printer << " " << boost::yap::op_string(K) << " ";
    boost::yap::transform_strict(expr_.right(), math_leaf_xform{_printer},
                                 math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate assignment.
  template <typename LHS, typename RHS>
  void operator()(expr<expr_kind::assign, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << " = ";
    boost::yap::transform_strict(expr_.right(), math_leaf_xform{_printer},
                                 math_expr_xform{_printer});
    _printer << ";\n";
  }

  /// Generate operator-assignment (+=, ...).
  template <expr_kind K, typename LHS, typename RHS, typename = void,
            typename = std::enable_if_t<is_opassign_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << ' ' << boost::yap::op_string(K) << ' ';
    boost::yap::transform_strict(expr_.right(), math_leaf_xform{_printer},
                                 math_expr_xform{_printer});
    _printer << ";\n";
  }

public:
  math_expr_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

// class statement_xform {{{

class statement_xform : ::prtcl::expr::xform_helper {
public:
  template <typename E>
  void operator()(term<::prtcl::expr::eq<E>> expr_) const {
    _printer.indent();
    boost::yap::transform_strict(expr_.value().expression,
                                 math_expr_xform{_printer.indented()});
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<::prtcl::expr::rd<RT, LHS, RHS>> expr_) const {
    // TODO: needs to be split into three parts, one called at the beginning
    // of the particle loop, one called whereever the reduction is happening
    // and one at the end of the particle loop
    _printer.indent();
    boost::yap::transform_strict(expr_.value().rhs,
                                 math_leaf_xform{_printer.indented()},
                                 math_expr_xform{_printer.indented()});
  }

public:
  explicit statement_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

// class neighbour_loop_xform {{{

class neighbour_loop_xform : ::prtcl::expr::xform_helper {
public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::neighbour_loop>, Exprs...> expr) {
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
        });
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    auto &selector = expr.elements[0_c].value();
    // loop over the selected groups
    _printer.indent() << "for (auto &n : _groups." << selector.type << ") {"
                      << '\n';
    _printer.increase_indent();
    // loop over the neighbour indices in the selected group
    _printer.indent() << "for (size_t j = 0; j < n._size; ++j) {" << '\n';
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e),
                                       statement_xform{_printer.indented()});
        });
    // close the loop over the particle indices
    _printer.indent() << "}" << '\n';
    // close the loop over the groups
    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';
  }

public:
  explicit neighbour_loop_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

// class particle_loop_xform {{{

class particle_loop_xform : ::prtcl::expr::xform_helper {
public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::particle_loop>, Exprs...> expr) {
    _printer << "#pragma omp parallel" << '\n';
    _printer.indent() << "{" << '\n';
    _printer.increase_indent();
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
        });
    _printer.decrease_indent();
    _printer.indent() << "}\n";
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    auto &selector = expr.elements[0_c].value();
    // loop over the selected groups
    _printer.indent() << "for (auto &p : _groups." << selector.type << ") {"
                      << '\n';
    _printer.increase_indent();
    // loop over the particle indices in the selected group
    _printer << "#pragma omp for" << '\n';
    _printer.indent() << "for (size_t i = 0; i < p._size; ++i) {" << '\n';
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e),
              statement_xform{_printer.indented()},
              neighbour_loop_xform{_printer.indented()});
        });
    // close the loop over the particle indices
    _printer.indent() << "}" << '\n';
    // close the loop over the groups
    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';
  }

public:
  explicit particle_loop_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

// class section_xform {{{

class section_xform : ::prtcl::expr::xform_helper {
public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::section>, Exprs...> expr) const {
    using namespace boost::hana::literals;
    _printer.indent() << "void " << expr.elements[0_c].value().name << "() {\n";
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e),
              statement_xform{_printer.indented()},
              particle_loop_xform{_printer.indented()});
        });
    _printer.indent() << "}\n";
  }

public:
  explicit section_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

} // namespace n_openmp

template <typename Expr>
void generate_openmp_source(std::ostream &stream_, std::string name_,
                            Expr &&expr_) {
  namespace omp = ::prtcl::n_openmp;
  omp::requirements reqs;
  // extract all requirements of the sections on the groups
  omp::extract_requirements(reqs, std::forward<Expr>(expr_));

  omp::cxx_printer printer{stream_};

  printer << "#pragma once" << '\n';
  printer << '\n';

  printer << "#include <prtcl/model_base.hpp>" << '\n';
  printer << '\n';
  printer << "#include <string_view>" << '\n';
  printer << "#include <vector>" << '\n';
  printer << '\n';
  printer << "#include <cstddef>" << '\n';

  printer << '\n';
  printer << "namespace prtcl::scheme {" << '\n';
  printer << '\n';

  printer.indent() << "template <typename T, size_t N> class " << name_
                   << " : private ::prtcl::model_base {" << '\n';
  printer.increase_indent();

  auto make_field_handler = [](auto &&callable_) {
    return ::prtcl::meta::overload{std::forward<decltype(callable_)>(callable_),
                                   [](std::monostate) {}};
  };

  // generate: struct { ... } g; {{{
  printer.indent() << "struct {" << '\n';
  printer.increase_indent();

  // generate code for the group types
  for (auto f_var : reqs.scheme_fields)
    std::visit(make_field_handler([&printer](auto f) {
                 auto &s = printer.indent();
                 if (f.type_tag == tag::type::scalar{})
                   s << "ndfield_ref_t<T>";
                 if (f.type_tag == tag::type::vector{})
                   s << "ndfield_ref_t<T, N>";
                 if (f.type_tag == tag::type::matrix{})
                   s << "ndfield_ref_t<T, N, N>";
                 s << " " << f.value << ";" << '\n';
               }),
               f_var);

  printer << '\n';
  // generate: static: require(...) {{{
  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "static void require(scheme_t<T, N> &s_) {" << '\n';
  printer.increase_indent();

  printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
  for (auto f_var : reqs.scheme_fields)
    std::visit(make_field_handler([&printer](auto f) {
                 printer.indent()
                     << "s_.add(\"" << f.value << "\"_g"
                     << boost::lexical_cast<std::string>(f.type_tag)[0] << "f);"
                     << '\n';
               }),
               f_var);

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  // }}}

  printer << '\n';
  // generate: member: load(...) {{{
  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "void load(scheme_t<T, N> &s_) {" << '\n';
  printer.increase_indent();

  printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
  for (auto f_var : reqs.scheme_fields)
    std::visit(make_field_handler([&printer](auto f) {
                 printer.indent()
                     << f.value << " = "
                     << "s_.get(\"" << f.value << "\"_g"
                     << boost::lexical_cast<std::string>(f.type_tag)[0] << "f);"
                     << '\n';
               }),
               f_var);

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  // }}}

  printer.decrease_indent();
  printer.indent() << "} g;" << '\n';
  // }}}

  printer << '\n';

  // generate: struct { ... } _groups; {{{
  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "struct {" << '\n';
  printer.increase_indent();

  for (auto [name, fields] : reqs.group_fields) {
    // generate: struct: ..._group_type {{{
    printer.indent() << "struct " << name << "_group_type {" << '\n';
    printer.increase_indent();

    // generate code for the group types
    for (auto f_var : fields)
      std::visit(make_field_handler([&printer](auto f) {
                   auto &s = printer.indent();
                   if (f.type_tag == tag::type::scalar{})
                     s << "ndfield_ref_t<T>";
                   if (f.type_tag == tag::type::vector{})
                     s << "ndfield_ref_t<T, N>";
                   if (f.type_tag == tag::type::matrix{})
                     s << "ndfield_ref_t<T, N, N>";
                   s << " " << f.value << ";" << '\n';
                 }),
                 f_var);

    printer << '\n';
    printer.indent() << "size_t _size;" << '\n';

    printer << '\n';
    // generate: static: require(...) {{{
    printer.decrease_indent();
    printer.indent() << "public:" << '\n';
    printer.increase_indent();

    printer.indent() << "static void require(group_t<T, N> &g_) {" << '\n';
    printer.increase_indent();

    printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
    for (auto f_var : fields)
      std::visit(make_field_handler([&printer](auto f) {
                   printer.indent()
                       << "g_.add(\"" << f.value << "\"_"
                       << boost::lexical_cast<std::string>(f.kind_tag)[0]
                       << boost::lexical_cast<std::string>(f.type_tag)[0]
                       << "f);" << '\n';
                 }),
                 f_var);

    printer.decrease_indent();
    printer.indent() << "}" << '\n';
    // }}}

    printer << '\n';
    // generate: member: load(...) {{{
    printer.decrease_indent();
    printer.indent() << "public:" << '\n';
    printer.increase_indent();

    printer.indent() << "void load(group_t<T, N> &g_) {" << '\n';
    printer.increase_indent();

    printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
    for (auto f_var : fields)
      std::visit(make_field_handler([&printer](auto f) {
                   printer.indent()
                       << f.value << " = "
                       << "g_.get(\"" << f.value << "\"_"
                       << boost::lexical_cast<std::string>(f.kind_tag)[0]
                       << boost::lexical_cast<std::string>(f.type_tag)[0]
                       << "f);" << '\n';
                 }),
                 f_var);

    printer << '\n';
    printer.indent() << "_size = g_.size();" << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';
    // }}}

    printer.decrease_indent();
    printer.indent() << "};" << '\n';
    // }}}

    printer << '\n';
    printer.indent() << "std::vector<" << name << "_group_type> " << name << ";"
                     << '\n';

    printer << '\n';
  }

  printer.decrease_indent();
  printer.indent() << "} _groups;" << '\n';
  // }}}

  printer << '\n';
  // generate: static: require(...) { ... } {{{

  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "static void require(scheme_t<T, N> &s_) {" << '\n';
  printer.increase_indent();

  printer.indent() << "decltype(g)::require(s_);" << '\n';

  for (auto [name, fields] : reqs.group_fields) {
    (void)(fields);
    printer.indent() << '\n';

    printer.indent() << "for (auto &group : s_.groups()) {" << '\n';
    printer.increase_indent();

    printer.indent() << "if (group.get_type() == \"" << name << "\") {" << '\n';
    printer.increase_indent();

    printer.indent() << "decltype(_groups)::" << name
                     << "_group_type::require(group);" << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';
  }

  printer.decrease_indent();
  printer.indent() << "}" << '\n';

  // }}}

  printer << '\n';
  // generate: member: load(...) { ... } {{{

  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "void load(scheme_t<T, N> &s_) {" << '\n';
  printer.increase_indent();

  printer.indent() << "g.load(s_);" << '\n';

  for (auto [name, fields] : reqs.group_fields) {
    (void)(fields);
    printer.indent() << '\n';

    printer.indent() << "_groups." << name << ".clear();" << '\n';

    printer.indent() << "for (auto &group : s_.groups()) {" << '\n';
    printer.increase_indent();

    printer.indent() << "if (group.get_type() == \"" << name << "\") {" << '\n';
    printer.increase_indent();

    printer.indent() << "_groups." << name << ".emplace_back().load(group);"
                     << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';
  }

  printer.decrease_indent();
  printer.indent() << "}" << '\n';

  // }}}

  printer << '\n';

  // generate code for all sections
  boost::yap::transform_strict(std::forward<Expr>(expr_),
                               omp::section_xform{printer});

  printer << '\n';

  // generate: static: xml() {{{
  printer.indent() << "static std::string_view xml() {" << '\n';
  printer.increase_indent();

  printer.indent() << "return R\"prtcl(" << '\n';
  expr::print(printer.stream(), std::forward<Expr>(expr_));
  printer << ")prtcl\";" << '\n';

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  // }}}

  printer.decrease_indent();
  printer.indent() << "};" << '\n';

  printer << '\n';
  printer << "} // namespace prtcl::scheme" << '\n';
}

} // namespace prtcl
