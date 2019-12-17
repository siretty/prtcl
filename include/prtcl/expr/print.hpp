#pragma once

#include <prtcl/expr/eq.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/loop.hpp>
#include <prtcl/expr/rd.hpp>
#include <prtcl/expr/section.hpp>
#include <prtcl/expr/transform/xform_helper.hpp>
#include <prtcl/meta/remove_cvref.hpp>
#include <prtcl/tag/call.hpp>
#include <prtcl/tag/group.hpp>

#include <ostream>
#include <type_traits>

#include <boost/hana.hpp>
#include <boost/hana/fwd/for_each.hpp>
#include <boost/yap/algorithm.hpp>
#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/print.hpp>

namespace prtcl::expr {

template <typename KT, typename TT>
decltype(auto) _print(std::ostream &stream_, field<KT, TT> const &field_) {
  return stream_ << "<field kind=\"" << field_.kind_tag << "\" type=\""
                 << field_.type_tag << "\">" << field_.value << "</field>";
}

template <typename GT, typename = std::enable_if_t<tag::is_group_v<GT>>>
decltype(auto) _print(std::ostream &stream_, GT &&) {
  return stream_ << "<group>" << meta::remove_cvref_t<GT>{} << "</group>";
}

class indented_printer {
  size_t _indent;
  std::ostream &_stream;

public:
  indented_printer(std::ostream &stream_) : indented_printer(0, stream_) {}

  indented_printer(size_t indent_, std::ostream &stream_)
      : _indent{indent_}, _stream{stream_} {}

public:
  decltype(auto) indent() const {
    for (size_t i = 0; i < this->_indent * 2; ++i)
      this->_stream.put(' ');
    return this->_stream;
  }

  decltype(auto) stream() const { return this->_stream; }

  template <typename RHS> decltype(auto) operator<<(RHS &&rhs) const {
    return _stream << std::forward<RHS>(rhs);
  }

  char const *newline_string() const { return "\n"; }

  auto indented() const { return indented_printer{_indent + 1, _stream}; }
};

class print_math_leaf_xform : protected xform_helper {
private:
  char const *nl() const { return _printer.newline_string(); }

public:
  template <typename KT, typename TT>
  void operator()(term<field<KT, TT>> const &term_) const {
    _print(_printer.indent(), term_.value()) << nl();
  }

  template <typename KT, typename TT, typename GT,
            typename = std::enable_if_t<tag::is_group_v<GT>>>
  void operator()(subs<term<field<KT, TT>>, term<GT>> expr_) const {
    _printer.indent() << "<subscript>" << nl();
    _print(_printer.indented().indent(), expr_.left().value()) << nl();
    _print(_printer.indented().indent(), expr_.right().value()) << nl();
    _printer.indent() << "</subscript>" << nl();
  }

public:
  print_math_leaf_xform(indented_printer printer_) : _printer{printer_} {}

private:
  indented_printer _printer;
};

class print_math_xform : xform_helper {
private:
  char const *nl() const { return _printer.newline_string(); }

public:
  template <typename T> void operator()(term<T> const &term_) const {
    _printer.indent() << "<value>" << term_.value() << "</value>" << nl();
  }

  template <typename CT, typename... Args>
  std::enable_if_t<tag::is_call_v<CT>>
  operator()(call_expr<term<CT>, Args...> expr_) const {
    using namespace boost::hana::literals;
    _printer.indent() << "<call name=\"" << expr_.elements[0_c].value() << "\">"
                      << nl();
    boost::hana::for_each(
        boost::hana::slice_c<1, sizeof...(Args) + 1>(expr_.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e),
              print_math_leaf_xform{_printer.indented()},
              print_math_xform{_printer.indented()});
        });
    _printer.indent() << "</call>" << nl();
  }

  template <expr_kind K, typename Expr,
            typename = std::enable_if_t<is_uop_v<K>>>
  void operator()(expr<K, Expr> const &expr_) const {
    using namespace boost::hana::literals;
    _printer.indent() << "<unary op=\"" << boost::yap::op_string(K) << "\">"
                      << nl();
    boost::yap::transform_strict(expr_.elements[0_c],
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</unary>" << nl();
  }

  template <expr_kind K, typename LHS, typename RHS,
            typename = std::enable_if_t<is_bop_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    _printer.indent() << "<binary op=\"" << boost::yap::op_string(K) << "\">"
                      << nl();
    boost::yap::transform_strict(expr_.left(),
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    boost::yap::transform_strict(expr_.right(),
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</binary>" << nl();
  }

  template <typename LHS, typename RHS>
  void operator()(expr<expr_kind::assign, LHS, RHS> const &expr_) const {
    _printer.indent() << "<assign>" << nl();
    boost::yap::transform_strict(expr_.left(),
                                 print_math_leaf_xform{_printer.indented()});
    boost::yap::transform_strict(expr_.right(),
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</assign>" << nl();
  }

  template <expr_kind K, typename LHS, typename RHS, typename = void,
            typename = std::enable_if_t<is_opassign_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    _printer.indent() << "<opassign op=\"" << boost::yap::op_string(K) << "\">"
                      << nl();
    boost::yap::transform_strict(expr_.left(),
                                 print_math_leaf_xform{_printer.indented()});
    boost::yap::transform_strict(expr_.right(),
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</opassign>" << nl();
  }

public:
  print_math_xform(indented_printer printer_) : _printer{printer_} {}

private:
  indented_printer _printer;
};

class print_xform : xform_helper {
private:
  char const *nl() const { return _printer.newline_string(); }

public:
  template <typename... Exprs>
  void operator()(call_expr<term<section>, Exprs...> expr) const {
    using namespace boost::hana::literals;

    _printer.indent() << "<section";
    if (0 < expr.elements[0_c].value().name.size())
      _printer << " name=\"" << expr.elements[0_c].value().name << "\"";
    _printer << ">" << nl();
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(e, print_xform{_printer.indented()});
        });
    _printer.indent() << "</section>" << nl();
  }

  template <typename E> void operator()(term<eq<E>> expr_) const {
    _printer.indent() << "<eq>" << nl();
    boost::yap::transform_strict(expr_.value().expression,
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</eq>" << nl();
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<rd<RT, LHS, RHS>> expr_) const {
    _printer.indent() << "<rd op=\"" << expr_.value().reduce_tag << "\">"
                      << nl();
    _print(_printer.indented().indent(), expr_.value().lhs) << nl();
    boost::yap::transform_strict(expr_.value().rhs,
                                 print_math_leaf_xform{_printer.indented()},
                                 print_math_xform{_printer.indented()});
    _printer.indent() << "</rd>" << nl();
  }

  template <typename... Exprs>
  void operator()(call_expr<term<selector>, Exprs...> expr) const {
    _printer.indent() << "<selector>" << nl();
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(e, print_xform{_printer.indented()});
        });
    _printer.indent() << "</selector>" << nl();
  }

  template <typename... Exprs>
  void operator()(call_expr<term<particle_loop>, Exprs...> expr) const {
    _printer.indent() << "<particle_loop>" << nl();
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(e, print_xform{_printer.indented()});
        });
    _printer.indent() << "</particle_loop>" << nl();
  }

  template <typename... Exprs>
  void operator()(call_expr<term<neighbour_loop>, Exprs...> expr) const {
    _printer.indent() << "<neighbour_loop>" << nl();
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          boost::yap::transform_strict(e, print_xform{_printer.indented()});
        });
    _printer.indent() << "</neighbour_loop>" << nl();
  }

public:
  explicit print_xform(std::ostream &stream_) : _printer{0, stream_} {}

private:
  explicit print_xform(indented_printer printer_) : _printer{printer_} {}

private:
  indented_printer _printer;
};

template <typename Expr> void print(std::ostream &stream_, Expr &&expr_) {
  boost::yap::transform_strict(std::forward<Expr>(expr_), print_xform{stream_});
}

} // namespace prtcl::expr
