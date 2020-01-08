#pragma once

#include <boost/bimap/multiset_of.hpp>
#include <prtcl/gt/eq.hpp>
#include <prtcl/gt/field.hpp>
#include <prtcl/gt/index_kind.hpp>
#include <prtcl/gt/rd.hpp>
#include <prtcl/gt/select_group_type.hpp>

#include <prtcl/gt/field_requirements.hpp>
#include <prtcl/gt/xform_helper.hpp>

#include <optional>
#include <ostream>

#include <cstddef>

#include <boost/hana.hpp>

#include <boost/bimap/bimap.hpp>

#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

namespace n_openmp {

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

  void decrease_indent() {
    if (0 == _indent)
      throw "cannot decrease zero indentation";
    --_indent;
  }

  auto indented() const {
    return cxx_printer{
        _stream,
        _indent + 1,
    };
  }
};

// }}}

// {{{ class reduction_requirements

class reduction_requirements {
public:
  void add_reduction(field f_, eq_op o_) {
    if (field_kind::varying == f_.kind())
      throw "invalid field_kind";
    if (eq_op::assign == o_)
      throw "invalid eq_op";

    auto [it, inserted] = _map.left.insert({f_, o_});
    if (not inserted)
      throw "cannot reduce the same field with different operations";
  }

public:
  auto reductions() const { return ::boost::make_iterator_range(_map); }

  auto global_reductions() const {
    return reductions() | boost::adaptors::filtered([](auto &&p_) {
             return field_kind::global ==
                    std::forward<decltype(p_)>(p_).second.kind();
           });
  }

  auto uniform_reductions() const {
    return reductions() | boost::adaptors::filtered([](auto &&p_) {
             return field_kind::uniform ==
                    std::forward<decltype(p_)>(p_).second.kind();
           });
  }

private:
  using field_rd_map = boost::bimaps::bimap<
      boost::bimaps::set_of<field>, boost::bimaps::multiset_of<eq_op>>;

private:
  field_rd_map _map;
};

// }}}

// class extract_reduction_requirements_xform {{{

class extract_reduction_requirements_xform : xform_helper {
public:
  template <typename Expr_> auto operator()(term<eq<Expr_>> term_) {
    auto e = term_.value();

    if (e.is_reduction())
      _reqs.add_reduction(e.field, e.op);

    return term_;
  }

public:
  extract_reduction_requirements_xform(
      reduction_requirements &reqs_, std::string cur_p_)
      : _reqs{reqs_}, _cur_p{cur_p_} {}

private:
  reduction_requirements &_reqs;
  std::string _cur_p;
};

// }}}

// extract_reduction_requirements(...) {{{

template <typename Expr>
static void extract_reduction_requirements(
    reduction_requirements &reqs_, std::string cur_p_, Expr &&expr_) {
  boost::yap::transform(
      std::forward<Expr>(expr_),
      extract_reduction_requirements_xform{reqs_, cur_p_});
}

// }}}

/*

// struct reduction_collection {{{

struct reduction_collection {
  using reduce_tag = std::variant<
      ::prtcl::tag::reduce::plus, ::prtcl::tag::reduce::minus,
      ::prtcl::tag::reduce::multiplies, ::prtcl::tag::reduce::divides,
      ::prtcl::tag::reduce::max, ::prtcl::tag::reduce::min>;

  using any_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::global, tag::type::matrix>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::matrix>>;

  using global_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::global, tag::type::matrix>>;

  using uniform_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::matrix>>;

  std::vector<any_field> fields;
  std::vector<global_field> global_fields;
  std::vector<reduce_tag> global_reduce_tags;
  std::unordered_map<std::string, std::vector<uniform_field>> uniform_fields;
  std::unordered_map<std::string, std::vector<reduce_tag>> uniform_reduce_tags;
};

// }}}

// extract_reductions(...) {{{

template <typename Expr>
static void extract_reductions(
    reduction_collection &reds_, std::string cur_p_, Expr &&expr_) {
  boost::yap::transform(
      std::forward<Expr>(expr_), extract_reductions_xform{reds_, cur_p_});

  // uniquify(...) {{{
  auto uniquify = [](auto &fields_, auto &tags_) {
    std::vector<size_t> permutation(fields_.size());
    ::boost::range::iota(permutation, 0);

    ::boost::range::sort(permutation, [&fields_](auto lhs_idx_, auto rhs_idx_) {
      return std::visit(
          [](auto lhs_, auto rhs_) -> bool {
            if constexpr (
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(lhs_)>, std::monostate> or
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(rhs_)>, std::monostate>)
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
          fields_[lhs_idx_], fields_[rhs_idx_]);
    });

    { // sort the fields
      std::vector<size_t> perm{permutation};
      ::boost::algorithm::apply_permutation(fields_, perm);
    }
    { // sort the tags
      std::vector<size_t> perm{permutation};
      ::boost::algorithm::apply_permutation(tags_, perm);
    }

    auto last = ::boost::range::unique(
                    ::boost::range::combine(fields_, tags_),
                    [](auto lhss_, auto rhss_) {
                      return boost::get<0>(lhss_) == boost::get<0>(rhss_);
                    })
                    .end();

    fields_.erase(boost::get<0>(last.get_iterator_tuple()), fields_.end());
    tags_.erase(boost::get<1>(last.get_iterator_tuple()), tags_.end());
  };
  // }}}

  {
    std::vector<char> dummy(reds_.fields.size());
    uniquify(reds_.fields, dummy);
  }

  uniquify(reds_.global_fields, reds_.global_reduce_tags);
  for (auto &pair : reds_.uniform_fields)
    uniquify(pair.second, reds_.uniform_reduce_tags[pair.first]);
}

// }}}

// class extract_selector_types_xform {{{

class extract_selector_types_xform : ::prtcl::expr::xform_helper {
public:
  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace ::boost::hana::literals;
    types.insert(expr.elements[0_c].value().type);
  }

  std::set<std::string> types;
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
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                  term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[0]";
  }

  /// Generate access to uniform field of the passive group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                  term<tag::group::passive>>
                      expr_) const {
    _printer << "n." << expr_.left().value().value << "[0]";
  }

  /// Generate access to varying field of the active group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::varying, TT>>,
                  term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[i]";
  }

  /// Generate access to varying field of the passive group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::varying, TT>>,
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

  /// Generate unary operator.
  template <
      expr_kind K, typename Expr, typename = std::enable_if_t<is_uop_v<K>>>
  void operator()(expr<K, Expr> const &expr_) const {
    using namespace boost::hana::literals;
    _printer << "( " << boost::yap::op_string(K);
    boost::yap::transform_strict(
        expr_.elements[0_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate binary operator.
  template <
      expr_kind K, typename LHS, typename RHS,
      typename = std::enable_if_t<is_bop_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    _printer << "( ";
    boost::yap::transform_strict(
        expr_.left(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << " " << boost::yap::op_string(K) << " ";
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate assignment.
  template <typename LHS, typename RHS>
  void operator()(expr<expr_kind::assign, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << " = ";
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << ";\n";
  }

  /// Generate operator-assignment (+=, ...).
  template <
      expr_kind K, typename LHS, typename RHS, typename = void,
      typename = std::enable_if_t<is_opassign_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << ' ' << boost::yap::op_string(K) << ' ';
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << ";\n";
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
      boost::yap::transform_strict(
          std::forward<decltype(e)>(e), math_leaf_xform{_printer},
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

  /// Generate code for the number of particles in the primary group.
  void operator()(call_expr<term<::prtcl::tag::call::particle_count>>) const {
    _printer << "p._size";
  }

  /// Generate code for the current number of neighbours in the current
  /// neighbour group.
  void operator()(call_expr<term<::prtcl::tag::call::neighbour_count>>) const {
    _printer << "neighbours[n._index].size()";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::dot>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().dot( ( ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ").matrix() )";
  }

  /// Generate code for vector norm.
  template <typename ARG>
  void operator()(call_expr<term<::prtcl::tag::call::norm>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().norm()";
  }

  /// Generate code for squared vector norm.
  template <typename ARG>
  void operator()(
      call_expr<term<::prtcl::tag::call::norm_squared>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().squaredNorm()";
  }

  /// Generate code for vector normalization.
  template <typename ARG>
  void
  operator()(call_expr<term<::prtcl::tag::call::normalized>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().normalized()";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::max>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "std::max<scalar_type>(";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ", ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ")";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::min>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "std::min<scalar_type>(";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ", ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ")";
  }

  /// Generate code for vector normalization.
  void operator()(call_expr<term<::prtcl::tag::call::zero_vector>>) const {
    using namespace ::boost::hana::literals;
    _printer << "vector_type::Zero()";
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
    boost::yap::transform_strict(
        expr_.value().expression, math_expr_xform{_printer.indented()});
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<::prtcl::expr::rd<RT, LHS, RHS>> expr_) const {
    namespace rd_tag = ::prtcl::tag::reduce;
    auto rd = expr_.value();
    auto field = rd.lhs.value();
    std::string rd_ident =
        std::string{"rd_"} +
        ::boost::lexical_cast<std::string>(field.kind_tag)[0] +
        ::boost::lexical_cast<std::string>(field.type_tag)[0] +
        / * (_cur_p ? "_" : "") + _cur_p.value_or("") + * /
        "_" + field.value;

    _printer.indent() << rd_ident << " ";

    if (rd_tag::plus{} == rd.reduce_tag)
      _printer << "+= ";
    else if (rd_tag::minus{} == rd.reduce_tag)
      _printer << "-= ";
    else if (rd_tag::multiplies{} == rd.reduce_tag)
      _printer << "*= ";
    else if (rd_tag::divides{} == rd.reduce_tag)
      _printer << "/= ";
    else if (rd_tag::min{} == rd.reduce_tag)
      _printer << "= min(" << rd_ident << ", ";
    else if (rd_tag::max{} == rd.reduce_tag)
      _printer << "= max(" << rd_ident << ", ";
    else
      throw "invalid reduce tag";

    boost::yap::transform_strict(
        expr_.value().rhs, math_leaf_xform{_printer.indented()},
        math_expr_xform{_printer.indented()});

    if constexpr (
        rd_tag::min{} == rd.reduce_tag or rd_tag::max{} == rd.reduce_tag)
      _printer << ");" << '\n';
    else
      _printer << ";" << '\n';
  }

public:
  explicit statement_xform(
      cxx_printer printer_, std::optional<std::string> cur_p_ = {})
      : _printer{printer_}, _cur_p{cur_p_} {}

private:
  cxx_printer _printer;
  std::optional<std::string> _cur_p;
};

// }}}

// class neighbour_loop_xform {{{

class neighbour_loop_xform : ::prtcl::expr::xform_helper {

public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::neighbour_loop>, Exprs...> expr) {
    // extract all selected types
    extract_selector_types_xform extractor;
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [&extractor](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e), extractor);
        });

    _printer.indent() << "if (!has_neighbours) {" << '\n';
    _printer.increase_indent();

    _printer.indent() << "nhood_.neighbours(p._index, i, [&neighbours](auto "
                         "n_index, auto j) {"
                      << '\n';
    _printer.increase_indent();

    _printer.indent() << "neighbours[n_index].push_back(j);" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "});" << '\n';
    _printer.indent() << "has_neighbours = true;" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';

    for (auto type : extractor.types) {
      _printer << '\n';

      // loop over neighbour groups
      _printer.indent() << "for (auto &n : _groups_" << type << ") {" << '\n';
      _printer.increase_indent();

      // loop over neighbour indices
      _printer.indent() << "for (size_t j : neighbours[n._index]) {" << '\n';
      _printer.increase_indent();

      // generate code for the selectors
      boost::hana::for_each(
          boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
              expr.elements),
          [this, &type](auto &&e) {
            using namespace boost::hana::literals;
            auto selector = std::forward<decltype(e)>(e).elements[0_c].value();
            if (type == selector.type)
              boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
          });

      // close loop over neighbour indices
      _printer.decrease_indent();
      _printer.indent() << "} // j" << '\n';

      // close loop over neighbour groups
      _printer.decrease_indent();
      _printer.indent() << "} // n" << '\n';
    }
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this, type = _cur_p](auto &&e) {
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer, type});
        });
  }

public:
  explicit neighbour_loop_xform(cxx_printer printer_, std::string cur_p_)
      : _printer{printer_}, _cur_p{cur_p_} {}

private:
  cxx_printer _printer;
  std::string _cur_p;
};

// }}}

// class particle_loop_xform {{{

class particle_loop_xform : ::prtcl::expr::xform_helper {

public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::particle_loop>, Exprs...> expr) {
    // extract all selected types and reductions
    extract_selector_types_xform extractor;
    reduction_collection reductions;
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [&extractor, &reductions](auto e) {
          using namespace ::boost::hana::literals;
          boost::yap::transform_strict(e, extractor);
          extract_reductions(reductions, e.elements[0_c].value().type, e);
        });

    // make_rd_name(...) {{{
    auto make_rd_name = [](auto field_, auto type_) {
      return std::string{"rd_"} +
             ::boost::lexical_cast<std::string>(field_.kind_tag)[0] +
             ::boost::lexical_cast<std::string>(field_.type_tag)[0] +
             / * (type_ ? "_" : "") + type_.value_or("") + * /
             "_" + field_.value;
    };
    // }}}

    _printer.indent() << "{ // foreach particle {{{" << '\n';
    _printer.increase_indent();

    { // generate_rd_storage {{{
      auto generate_rd_storage =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      printer.indent() << "std::vector<" << f.type_tag
                                       << "_type> per_thread_"
                                       << make_rd_name(f, type) << ";" << '\n';
                    }},
                f_var_);
          };

      for (auto &f_var : reductions.fields)
        generate_rd_storage(f_var, {});

      // TODO: implement per-type reductions if neccessary
      // for (auto &f_var : reductions.global_fields)
      //  generate_rd_storage(f_var, {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto &f_var : pair.second)
      //    generate_rd_storage(f_var, pair.first);
    } // }}}

    _printer << '\n';

    // start a parallel region
    _printer.indent() << "#pragma omp parallel" << '\n';
    _printer.indent() << "{" << '\n';
    _printer.increase_indent();

    _printer.indent() << "#pragma omp single" << '\n';
    _printer.indent() << "{" << '\n';

    _printer.increase_indent();
    _printer.indent() << "auto const thread_count = "
                         "static_cast<size_t>(omp_get_num_threads());"
                      << '\n';
    _printer.indent() << "_per_thread_neighbours.resize(thread_count);" << '\n';

    { // generate_rd_resize {{{
      auto generate_rd_resize =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      printer.indent() << "per_thread_" << make_rd_name(f, type)
                                       << ".resize(thread_count);" << '\n';
                    }},
                f_var_);
          };

      for (auto &f_var : reductions.fields)
        generate_rd_resize(f_var, {});
      // for (auto &f_var : reductions.global_fields)
      //  generate_rd_resize(f_var, {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto &f_var : pair.second)
      //    generate_rd_resize(f_var, pair.first);
    } // }}}

    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';

    _printer << '\n';

    _printer.indent() << "auto const thread_index = "
                         "static_cast<size_t>(omp_get_thread_num());"
                      << '\n';

    _printer << '\n';

    _printer.indent()
        << "// select and resize the neighbour storage for the current thread"
        << '\n';
    _printer.indent()
        << "auto &neighbours = _per_thread_neighbours[thread_index];" << '\n';
    _printer.indent() << "neighbours.resize(_group_count);" << '\n';
    _printer << '\n';
    _printer.indent() << "// reserve space for neighbours of each group"
                      << '\n';
    _printer.indent() << "for (auto &pgn : neighbours)" << '\n';
    _printer.increase_indent();
    _printer.indent() << "pgn.reserve(100);" << '\n';
    _printer.decrease_indent();

    if (reductions.fields.size()) {
      _printer << '\n';

      _printer.indent() << "// per-thread reduction variables" << '\n';
      // thread local reduction variable aliases {{{
      auto generate_rd_alias =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      auto name = make_rd_name(f, type);
                      printer.indent() << "auto &" << name << " = per_thread_"
                                       << name << "[thread_index];" << '\n';
                    }},
                f_var_);
            // TODO: check if needed {{{
            // auto generate_rd_alias = [&printer = _printer, &make_rd_name](
            //                             auto &f_var_, auto &t_var_,
            //                             std::optional<std::string> type_) {
            //  std::visit(
            //      ::prtcl::meta::overload{
            //          [](std::monostate, auto) {},
            //          [&printer, &type = type_, &make_rd_name](auto f, auto) {
            //            namespace rd_tag = ::prtcl::tag::reduce;
            //            auto name = make_rd_name(f, type);
            //            printer.indent() << "auto &" << name << " =
            //            per_thread_"
            //                             << name << "[thread_index];" << '\n';
            //          }},
            //      f_var_, t_var_);
            // }}}
          };

      for (auto f_var : reductions.fields)
        generate_rd_alias(f_var, {});
      // for (auto both : boost::range::combine(
      //         reductions.global_fields, reductions.global_reduce_tags))
      //  generate_rd_alias(boost::get<0>(both), boost::get<1>(both), {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto both : boost::range::combine(
      //           pair.second, reductions.uniform_reduce_tags[pair.first]))
      //    generate_rd_alias(
      //        boost::get<0>(both), boost::get<1>(both), pair.first);
      // }}}
    }

    // generate_rd_init(...) {{{
    auto generate_rd_init = [&printer = _printer, &make_rd_name](
                                auto &f_var_, auto &t_var_,
                                std::optional<std::string> type_) {
      std::visit(
          ::prtcl::meta::overload{
              [](std::monostate, auto) {},
              [&printer, &type = type_, &make_rd_name](auto f, auto t) {
                namespace rd_tag = ::prtcl::tag::reduce;
                auto name = make_rd_name(f, type);
                std::string initializer;
                if constexpr (rd_tag::plus{} == t or rd_tag::minus{} == t)
                  initializer = "0";
                else if constexpr (
                    rd_tag::multiplies{} == t or rd_tag::divides{} == t)
                  initializer = "1";
                else if constexpr (rd_tag::min{} == t)
                  initializer = "std::numeric_limits<scalar_type>::max()";
                else if constexpr (rd_tag::max{} == t)
                  initializer = "std::numeric_limits<scalar_type>::lowest()";
                else
                  throw "invalid reduce tag";
                printer.indent() << name << " = " << initializer << ";" << '\n';
                printer.indent() << "#pragma omp single" << '\n';
                printer.indent() << "{" << '\n';
                printer.increase_indent();
                if (f.kind_tag == ::prtcl::tag::kind::global{})
                  printer.indent() << "g.";
                else if (f.kind_tag == ::prtcl::tag::kind::uniform{})
                  printer.indent() << "p.";
                printer << f.value << "[0] = " << initializer << ";" << '\n';
                printer.decrease_indent();
                printer.indent() << "}" << '\n';
              }},
          f_var_, t_var_);
    };
    // }}}
    // generate_rd_combine(...) {{{
    auto generate_rd_combine = [&printer = _printer, &make_rd_name](
                                   auto &f_var_, auto &t_var_,
                                   std::optional<std::string> type_) {
      std::visit(
          ::prtcl::meta::overload{
              [](std::monostate, auto) {},
              [&printer, &type = type_, &make_rd_name](auto f, auto t) {
                std::string field_ident;
                if (!type)
                  field_ident = "g." + f.value;
                else
                  field_ident = "p." + f.value;
                printer.indent() << field_ident << "[0] ";
                namespace rd_tag = ::prtcl::tag::reduce;
                auto name = make_rd_name(f, type);
                if constexpr (rd_tag::plus{} == t)
                  printer << "+= " << name << ";" << '\n';
                else if constexpr (rd_tag::minus{} == t)
                  printer << "-= " << name << ";" << '\n';
                else if constexpr (rd_tag::multiplies{} == t)
                  printer << "*= " << name << ";" << '\n';
                else if constexpr (rd_tag::divides{} == t)
                  printer << "/= " << name << ";" << '\n';
                else if constexpr (rd_tag::min{} == t)
                  printer << " = std::min(" << field_ident << "[0], " << name
                          << ");" << '\n';
                else if constexpr (rd_tag::max{} == t)
                  printer << " = std::max(" << field_ident << "[0], " << name
                          << ");" << '\n';
                else
                  throw "invalid reduce tag";
              }},
          f_var_, t_var_);
    };
    // }}}

    if (reductions.global_fields.size()) {
      _printer << '\n';

      _printer.indent() << "// initialize global reduction variables" << '\n';
      // generate_rd_init {{{
      for (auto both : boost::range::combine(
               reductions.global_fields, reductions.global_reduce_tags))
        generate_rd_init(boost::get<0>(both), boost::get<1>(both), {});
      // }}}
    }

    _printer << '\n';

    // for each selected type
    for (auto type : extractor.types) {
      // loop over particle groups
      _printer.indent() << "for (auto &p : _groups_" << type << ") {" << '\n';
      _printer.increase_indent();

      if (reductions.uniform_fields[type].size()) {
        _printer.indent() << "// initialize uniform reduction variables"
                          << '\n';
        // generate_rd_init {{{
        for (auto both : boost::range::combine(
                 reductions.uniform_fields[type],
                 reductions.uniform_reduce_tags[type]))
          generate_rd_init(boost::get<0>(both), boost::get<1>(both), type);
        // }}}
      }

      _printer << '\n';

      // loop over particle indices
      _printer.indent() << "#pragma omp for" << '\n';
      _printer.indent() << "for (size_t i = 0; i < p._size; ++i) {" << '\n';
      _printer.increase_indent();

      _printer.indent() << "for (auto &pgn : neighbours)" << '\n';
      _printer.increase_indent();
      _printer.indent() << "pgn.clear();" << '\n';
      _printer.decrease_indent();

      _printer << '\n';

      // lazy check if neighbours of this particle have been computed already
      _printer.indent() << "bool has_neighbours = false;" << '\n';
      _printer.indent() << "(void)(has_neighbours);" << '\n';

      // generate code for the selectors
      boost::hana::for_each(
          boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
              expr.elements),
          [this, &type](auto &&e) {
            using namespace boost::hana::literals;
            auto selector = std::forward<decltype(e)>(e).elements[0_c].value();
            if (type == selector.type)
              boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
          });

      // close loop over particle indices
      _printer.decrease_indent();
      _printer.indent() << "} // i" << '\n';

      if (reductions.uniform_fields[type].size()) {
        _printer << '\n';
        _printer.indent() << "// combine reduction variables" << '\n';
        // generate_rd_combine {{{
        _printer.indent() << "#pragma omp critical" << '\n';
        _printer.indent() << "{" << '\n';
        _printer.increase_indent();

        for (auto both : boost::range::combine(
                 reductions.uniform_fields[type],
                 reductions.uniform_reduce_tags[type]))
          generate_rd_combine(boost::get<0>(both), boost::get<1>(both), type);

        _printer.decrease_indent();
        _printer.indent() << "}" << '\n';
        // }}}
      }

      // close loop over particle groups
      _printer.decrease_indent();
      _printer.indent() << "} // p" << '\n';
    }

    if (reductions.global_fields.size()) {
      _printer << '\n';
      _printer.indent() << "// combine reduction variables" << '\n';
      // generate_rd_combine {{{
      _printer.indent() << "#pragma omp critical" << '\n';
      _printer.indent() << "{" << '\n';
      _printer.increase_indent();

      for (auto both : boost::range::combine(
               reductions.global_fields, reductions.global_reduce_tags))
        generate_rd_combine(boost::get<0>(both), boost::get<1>(both), {});

      _printer.decrease_indent();
      _printer.indent() << "}" << '\n';
      // }}}
    }

    // close the parallel region
    _printer.decrease_indent();
    _printer.indent() << "} // omp parallel" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "} // }}}" << '\n';
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this, type = expr.elements[0_c].value().type](auto &&e) {
          _printer << '\n';
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer, type},
              neighbour_loop_xform{_printer, type});
        });
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
  void operator()(call_expr<term<::prtcl::expr::section>, Exprs...> expr) {
    using namespace boost::hana::literals;
    _printer.indent() << "template <typename NHood>" << '\n';
    _printer.indent() << "void " << expr.elements[0_c].value().name
                      << "(NHood const &nhood_) {\n";
    _printer.increase_indent();

    _printer.indent() << "(void)(nhood_);" << '\n';
    _printer << '\n';

    _printer.indent() << "auto &g = _global;" << '\n';
    _printer.indent() << "(void)(g);" << '\n';

    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          _printer << '\n';
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer},
              particle_loop_xform{_printer});
        });
    _printer.decrease_indent();
    _printer.indent() << "}\n";
  }

public:
  explicit section_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

 */

} // namespace n_openmp

} // namespace prtcl::gt

/*

#pragma once

#include "prtcl/expr/loop.hpp"
#include "prtcl/tag/call.hpp"
#include "prtcl/tag/reduce.hpp"
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

#include <boost/algorithm/apply_permutation.hpp>
#include <boost/hana.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/combine.hpp>

namespace prtcl {

namespace n_openmp {

// struct requirements {{{

struct requirements {
  using scheme_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::global, tag::type::matrix>>;
  using group_field = std::variant<
      std::monostate,
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

  /// Record a uniform field (only from rd's).
  template <typename TT>
  auto operator()(term<::prtcl::expr::field<tag::kind::uniform, TT>> term_) {
    // throws if _cur_p is not set
    auto [it, inserted] = _reqs.group_fields.insert({_cur_p.value(), {}});
    it->second.emplace_back(term_.value());
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
    boost::yap::transform(expr_.value().lhs, *this);
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
  boost::yap::transform(
      std::forward<Expr>(expr_), extract_requirements_xform{reqs_});

  // uniquify(...) {{{
  auto uniquify = [](auto &fields_) {
    std::sort(fields_.begin(), fields_.end(), [](auto lhs_var_, auto rhs_var_) {
      return std::visit(
          [](auto lhs_, auto rhs_) -> bool {
            if constexpr (
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(lhs_)>, std::monostate> or
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(rhs_)>, std::monostate>)
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

// struct reduction_collection {{{

struct reduction_collection {
  using reduce_tag = std::variant<
      ::prtcl::tag::reduce::plus, ::prtcl::tag::reduce::minus,
      ::prtcl::tag::reduce::multiplies, ::prtcl::tag::reduce::divides,
      ::prtcl::tag::reduce::max, ::prtcl::tag::reduce::min>;

  using any_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::global, tag::type::matrix>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::matrix>>;

  using global_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::global, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::global, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::global, tag::type::matrix>>;

  using uniform_field = std::variant<
      std::monostate,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::scalar>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::vector>,
      ::prtcl::expr::field<tag::kind::uniform, tag::type::matrix>>;

  std::vector<any_field> fields;
  std::vector<global_field> global_fields;
  std::vector<reduce_tag> global_reduce_tags;
  std::unordered_map<std::string, std::vector<uniform_field>> uniform_fields;
  std::unordered_map<std::string, std::vector<reduce_tag>> uniform_reduce_tags;
};

// }}}

// class extract_reductions_xform {{{

class extract_reductions_xform : ::prtcl::expr::xform_helper {
public:
  template <typename RT, typename LHS, typename RHS>
  auto operator()(term<::prtcl::expr::rd<RT, LHS, RHS>> term_) {
    namespace tag = ::prtcl::tag;
    auto field = term_.value().lhs.value();
    _reds.fields.emplace_back(field);
    if constexpr (tag::kind::global{} == field.kind_tag) {
      _reds.global_fields.emplace_back(field);
      _reds.global_reduce_tags.emplace_back(term_.value().reduce_tag);
    } else if constexpr (tag::kind::uniform{} == field.kind_tag) {
      _reds.uniform_fields[_cur_p].emplace_back(field);
      _reds.uniform_reduce_tags[_cur_p].emplace_back(term_.value().reduce_tag);
    } else
      throw "invalid reduction field";
    return term_;
  }

public:
  extract_reductions_xform(reduction_collection &reds_, std::string cur_p_)
      : _reds{reds_}, _cur_p{cur_p_} {}

private:
  reduction_collection &_reds;
  std::string _cur_p;
};

// }}}

// extract_reductions(...) {{{

template <typename Expr>
static void extract_reductions(
    reduction_collection &reds_, std::string cur_p_, Expr &&expr_) {
  boost::yap::transform(
      std::forward<Expr>(expr_), extract_reductions_xform{reds_, cur_p_});

  // uniquify(...) {{{
  auto uniquify = [](auto &fields_, auto &tags_) {
    std::vector<size_t> permutation(fields_.size());
    ::boost::range::iota(permutation, 0);

    ::boost::range::sort(permutation, [&fields_](auto lhs_idx_, auto rhs_idx_) {
      return std::visit(
          [](auto lhs_, auto rhs_) -> bool {
            if constexpr (
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(lhs_)>, std::monostate> or
                meta::is_any_of_v<
                    meta::remove_cvref_t<decltype(rhs_)>, std::monostate>)
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
          fields_[lhs_idx_], fields_[rhs_idx_]);
    });

    { // sort the fields
      std::vector<size_t> perm{permutation};
      ::boost::algorithm::apply_permutation(fields_, perm);
    }
    { // sort the tags
      std::vector<size_t> perm{permutation};
      ::boost::algorithm::apply_permutation(tags_, perm);
    }

    auto last = ::boost::range::unique(
                    ::boost::range::combine(fields_, tags_),
                    [](auto lhss_, auto rhss_) {
                      return boost::get<0>(lhss_) == boost::get<0>(rhss_);
                    })
                    .end();

    fields_.erase(boost::get<0>(last.get_iterator_tuple()), fields_.end());
    tags_.erase(boost::get<1>(last.get_iterator_tuple()), tags_.end());
  };
  // }}}

  {
    std::vector<char> dummy(reds_.fields.size());
    uniquify(reds_.fields, dummy);
  }

  uniquify(reds_.global_fields, reds_.global_reduce_tags);
  for (auto &pair : reds_.uniform_fields)
    uniquify(pair.second, reds_.uniform_reduce_tags[pair.first]);
}

// }}}

// class extract_selector_types_xform {{{

class extract_selector_types_xform : ::prtcl::expr::xform_helper {
public:
  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace ::boost::hana::literals;
    types.insert(expr.elements[0_c].value().type);
  }

  std::set<std::string> types;
};

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

  void decrease_indent() {
    if (0 == _indent)
      throw "cannot decrease zero indentation";
    --_indent;
  }

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
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                  term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[0]";
  }

  /// Generate access to uniform field of the passive group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::uniform, TT>>,
                  term<tag::group::passive>>
                      expr_) const {
    _printer << "n." << expr_.left().value().value << "[0]";
  }

  /// Generate access to varying field of the active group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::varying, TT>>,
                  term<tag::group::active>>
                      expr_) const {
    _printer << "p." << expr_.left().value().value << "[i]";
  }

  /// Generate access to varying field of the passive group.
  template <typename TT>
  void operator()(subs<
                  term<::prtcl::expr::field<tag::kind::varying, TT>>,
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

  /// Generate unary operator.
  template <
      expr_kind K, typename Expr, typename = std::enable_if_t<is_uop_v<K>>>
  void operator()(expr<K, Expr> const &expr_) const {
    using namespace boost::hana::literals;
    _printer << "( " << boost::yap::op_string(K);
    boost::yap::transform_strict(
        expr_.elements[0_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate binary operator.
  template <
      expr_kind K, typename LHS, typename RHS,
      typename = std::enable_if_t<is_bop_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    _printer << "( ";
    boost::yap::transform_strict(
        expr_.left(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << " " << boost::yap::op_string(K) << " ";
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << " )";
  }

  /// Generate assignment.
  template <typename LHS, typename RHS>
  void operator()(expr<expr_kind::assign, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << " = ";
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << ";\n";
  }

  /// Generate operator-assignment (+=, ...).
  template <
      expr_kind K, typename LHS, typename RHS, typename = void,
      typename = std::enable_if_t<is_opassign_v<K>>>
  void operator()(expr<K, LHS, RHS> const &expr_) const {
    boost::yap::transform_strict(expr_.left(), math_leaf_xform{_printer});
    _printer << ' ' << boost::yap::op_string(K) << ' ';
    boost::yap::transform_strict(
        expr_.right(), math_leaf_xform{_printer}, math_expr_xform{_printer});
    _printer << ";\n";
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
      boost::yap::transform_strict(
          std::forward<decltype(e)>(e), math_leaf_xform{_printer},
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

  /// Generate code for the number of particles in the primary group.
  void operator()(call_expr<term<::prtcl::tag::call::particle_count>>) const {
    _printer << "p._size";
  }

  /// Generate code for the current number of neighbours in the current
  /// neighbour group.
  void operator()(call_expr<term<::prtcl::tag::call::neighbour_count>>) const {
    _printer << "neighbours[n._index].size()";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::dot>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().dot( ( ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ").matrix() )";
  }

  /// Generate code for vector norm.
  template <typename ARG>
  void operator()(call_expr<term<::prtcl::tag::call::norm>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().norm()";
  }

  /// Generate code for squared vector norm.
  template <typename ARG>
  void operator()(
      call_expr<term<::prtcl::tag::call::norm_squared>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().squaredNorm()";
  }

  /// Generate code for vector normalization.
  template <typename ARG>
  void
  operator()(call_expr<term<::prtcl::tag::call::normalized>, ARG> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "( ";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << " ).matrix().normalized()";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::max>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "std::max<scalar_type>(";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ", ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ")";
  }

  /// Generate code for dot-products.
  template <typename LHS, typename RHS>
  void
  operator()(call_expr<term<::prtcl::tag::call::min>, LHS, RHS> call_) const {
    using namespace ::boost::hana::literals;
    _printer << "std::min<scalar_type>(";
    boost::yap::transform_strict(
        call_.elements[1_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ", ";
    boost::yap::transform_strict(
        call_.elements[2_c], math_leaf_xform{_printer},
        math_expr_xform{_printer});
    _printer << ")";
  }

  /// Generate code for vector normalization.
  void operator()(call_expr<term<::prtcl::tag::call::zero_vector>>) const {
    using namespace ::boost::hana::literals;
    _printer << "vector_type::Zero()";
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
    boost::yap::transform_strict(
        expr_.value().expression, math_expr_xform{_printer.indented()});
  }

  template <typename RT, typename LHS, typename RHS>
  void operator()(term<::prtcl::expr::rd<RT, LHS, RHS>> expr_) const {
    namespace rd_tag = ::prtcl::tag::reduce;
    auto rd = expr_.value();
    auto field = rd.lhs.value();
    std::string rd_ident =
        std::string{"rd_"} +
        ::boost::lexical_cast<std::string>(field.kind_tag)[0] +
        ::boost::lexical_cast<std::string>(field.type_tag)[0] +
        / * (_cur_p ? "_" : "") + _cur_p.value_or("") + * /
        "_" + field.value;

    _printer.indent() << rd_ident << " ";

    if (rd_tag::plus{} == rd.reduce_tag)
      _printer << "+= ";
    else if (rd_tag::minus{} == rd.reduce_tag)
      _printer << "-= ";
    else if (rd_tag::multiplies{} == rd.reduce_tag)
      _printer << "*= ";
    else if (rd_tag::divides{} == rd.reduce_tag)
      _printer << "/= ";
    else if (rd_tag::min{} == rd.reduce_tag)
      _printer << "= min(" << rd_ident << ", ";
    else if (rd_tag::max{} == rd.reduce_tag)
      _printer << "= max(" << rd_ident << ", ";
    else
      throw "invalid reduce tag";

    boost::yap::transform_strict(
        expr_.value().rhs, math_leaf_xform{_printer.indented()},
        math_expr_xform{_printer.indented()});

    if constexpr (
        rd_tag::min{} == rd.reduce_tag or rd_tag::max{} == rd.reduce_tag)
      _printer << ");" << '\n';
    else
      _printer << ";" << '\n';
  }

public:
  explicit statement_xform(
      cxx_printer printer_, std::optional<std::string> cur_p_ = {})
      : _printer{printer_}, _cur_p{cur_p_} {}

private:
  cxx_printer _printer;
  std::optional<std::string> _cur_p;
};

// }}}

// class neighbour_loop_xform {{{

class neighbour_loop_xform : ::prtcl::expr::xform_helper {

public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::neighbour_loop>, Exprs...> expr) {
    // extract all selected types
    extract_selector_types_xform extractor;
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [&extractor](auto &&e) {
          boost::yap::transform_strict(std::forward<decltype(e)>(e), extractor);
        });

    _printer.indent() << "if (!has_neighbours) {" << '\n';
    _printer.increase_indent();

    _printer.indent() << "nhood_.neighbours(p._index, i, [&neighbours](auto "
                         "n_index, auto j) {"
                      << '\n';
    _printer.increase_indent();

    _printer.indent() << "neighbours[n_index].push_back(j);" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "});" << '\n';
    _printer.indent() << "has_neighbours = true;" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';

    for (auto type : extractor.types) {
      _printer << '\n';

      // loop over neighbour groups
      _printer.indent() << "for (auto &n : _groups_" << type << ") {" << '\n';
      _printer.increase_indent();

      // loop over neighbour indices
      _printer.indent() << "for (size_t j : neighbours[n._index]) {" << '\n';
      _printer.increase_indent();

      // generate code for the selectors
      boost::hana::for_each(
          boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
              expr.elements),
          [this, &type](auto &&e) {
            using namespace boost::hana::literals;
            auto selector = std::forward<decltype(e)>(e).elements[0_c].value();
            if (type == selector.type)
              boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
          });

      // close loop over neighbour indices
      _printer.decrease_indent();
      _printer.indent() << "} // j" << '\n';

      // close loop over neighbour groups
      _printer.decrease_indent();
      _printer.indent() << "} // n" << '\n';
    }
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this, type = _cur_p](auto &&e) {
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer, type});
        });
  }

public:
  explicit neighbour_loop_xform(cxx_printer printer_, std::string cur_p_)
      : _printer{printer_}, _cur_p{cur_p_} {}

private:
  cxx_printer _printer;
  std::string _cur_p;
};

// }}}

// class particle_loop_xform {{{

class particle_loop_xform : ::prtcl::expr::xform_helper {

public:
  template <typename... Exprs>
  void
  operator()(call_expr<term<::prtcl::expr::particle_loop>, Exprs...> expr) {
    // extract all selected types and reductions
    extract_selector_types_xform extractor;
    reduction_collection reductions;
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [&extractor, &reductions](auto e) {
          using namespace ::boost::hana::literals;
          boost::yap::transform_strict(e, extractor);
          extract_reductions(reductions, e.elements[0_c].value().type, e);
        });

    // make_rd_name(...) {{{
    auto make_rd_name = [](auto field_, auto type_) {
      return std::string{"rd_"} +
             ::boost::lexical_cast<std::string>(field_.kind_tag)[0] +
             ::boost::lexical_cast<std::string>(field_.type_tag)[0] +
             / * (type_ ? "_" : "") + type_.value_or("") + * /
             "_" + field_.value;
    };
    // }}}

    _printer.indent() << "{ // foreach particle {{{" << '\n';
    _printer.increase_indent();

    { // generate_rd_storage {{{
      auto generate_rd_storage =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      printer.indent() << "std::vector<" << f.type_tag
                                       << "_type> per_thread_"
                                       << make_rd_name(f, type) << ";" << '\n';
                    }},
                f_var_);
          };

      for (auto &f_var : reductions.fields)
        generate_rd_storage(f_var, {});

      // TODO: implement per-type reductions if neccessary
      // for (auto &f_var : reductions.global_fields)
      //  generate_rd_storage(f_var, {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto &f_var : pair.second)
      //    generate_rd_storage(f_var, pair.first);
    } // }}}

    _printer << '\n';

    // start a parallel region
    _printer.indent() << "#pragma omp parallel" << '\n';
    _printer.indent() << "{" << '\n';
    _printer.increase_indent();

    _printer.indent() << "#pragma omp single" << '\n';
    _printer.indent() << "{" << '\n';

    _printer.increase_indent();
    _printer.indent() << "auto const thread_count = "
                         "static_cast<size_t>(omp_get_num_threads());"
                      << '\n';
    _printer.indent() << "_per_thread_neighbours.resize(thread_count);" << '\n';

    { // generate_rd_resize {{{
      auto generate_rd_resize =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      printer.indent() << "per_thread_" << make_rd_name(f, type)
                                       << ".resize(thread_count);" << '\n';
                    }},
                f_var_);
          };

      for (auto &f_var : reductions.fields)
        generate_rd_resize(f_var, {});
      // for (auto &f_var : reductions.global_fields)
      //  generate_rd_resize(f_var, {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto &f_var : pair.second)
      //    generate_rd_resize(f_var, pair.first);
    } // }}}

    _printer.decrease_indent();
    _printer.indent() << "}" << '\n';

    _printer << '\n';

    _printer.indent() << "auto const thread_index = "
                         "static_cast<size_t>(omp_get_thread_num());"
                      << '\n';

    _printer << '\n';

    _printer.indent()
        << "// select and resize the neighbour storage for the current thread"
        << '\n';
    _printer.indent()
        << "auto &neighbours = _per_thread_neighbours[thread_index];" << '\n';
    _printer.indent() << "neighbours.resize(_group_count);" << '\n';
    _printer << '\n';
    _printer.indent() << "// reserve space for neighbours of each group"
                      << '\n';
    _printer.indent() << "for (auto &pgn : neighbours)" << '\n';
    _printer.increase_indent();
    _printer.indent() << "pgn.reserve(100);" << '\n';
    _printer.decrease_indent();

    if (reductions.fields.size()) {
      _printer << '\n';

      _printer.indent() << "// per-thread reduction variables" << '\n';
      // thread local reduction variable aliases {{{
      auto generate_rd_alias =
          [&printer = _printer,
           &make_rd_name](auto &f_var_, std::optional<std::string> type_) {
            std::visit(
                ::prtcl::meta::overload{
                    [](std::monostate) {},
                    [&printer, &type = type_, &make_rd_name](auto f) {
                      auto name = make_rd_name(f, type);
                      printer.indent() << "auto &" << name << " = per_thread_"
                                       << name << "[thread_index];" << '\n';
                    }},
                f_var_);
            // TODO: check if needed {{{
            // auto generate_rd_alias = [&printer = _printer, &make_rd_name](
            //                             auto &f_var_, auto &t_var_,
            //                             std::optional<std::string> type_) {
            //  std::visit(
            //      ::prtcl::meta::overload{
            //          [](std::monostate, auto) {},
            //          [&printer, &type = type_, &make_rd_name](auto f, auto) {
            //            namespace rd_tag = ::prtcl::tag::reduce;
            //            auto name = make_rd_name(f, type);
            //            printer.indent() << "auto &" << name << " =
            //            per_thread_"
            //                             << name << "[thread_index];" << '\n';
            //          }},
            //      f_var_, t_var_);
            // }}}
          };

      for (auto f_var : reductions.fields)
        generate_rd_alias(f_var, {});
      // for (auto both : boost::range::combine(
      //         reductions.global_fields, reductions.global_reduce_tags))
      //  generate_rd_alias(boost::get<0>(both), boost::get<1>(both), {});
      // for (auto &pair : reductions.uniform_fields)
      //  for (auto both : boost::range::combine(
      //           pair.second, reductions.uniform_reduce_tags[pair.first]))
      //    generate_rd_alias(
      //        boost::get<0>(both), boost::get<1>(both), pair.first);
      // }}}
    }

    // generate_rd_init(...) {{{
    auto generate_rd_init = [&printer = _printer, &make_rd_name](
                                auto &f_var_, auto &t_var_,
                                std::optional<std::string> type_) {
      std::visit(
          ::prtcl::meta::overload{
              [](std::monostate, auto) {},
              [&printer, &type = type_, &make_rd_name](auto f, auto t) {
                namespace rd_tag = ::prtcl::tag::reduce;
                auto name = make_rd_name(f, type);
                std::string initializer;
                if constexpr (rd_tag::plus{} == t or rd_tag::minus{} == t)
                  initializer = "0";
                else if constexpr (
                    rd_tag::multiplies{} == t or rd_tag::divides{} == t)
                  initializer = "1";
                else if constexpr (rd_tag::min{} == t)
                  initializer = "std::numeric_limits<scalar_type>::max()";
                else if constexpr (rd_tag::max{} == t)
                  initializer = "std::numeric_limits<scalar_type>::lowest()";
                else
                  throw "invalid reduce tag";
                printer.indent() << name << " = " << initializer << ";" << '\n';
                printer.indent() << "#pragma omp single" << '\n';
                printer.indent() << "{" << '\n';
                printer.increase_indent();
                if (f.kind_tag == ::prtcl::tag::kind::global{})
                  printer.indent() << "g.";
                else if (f.kind_tag == ::prtcl::tag::kind::uniform{})
                  printer.indent() << "p.";
                printer << f.value << "[0] = " << initializer << ";" << '\n';
                printer.decrease_indent();
                printer.indent() << "}" << '\n';
              }},
          f_var_, t_var_);
    };
    // }}}
    // generate_rd_combine(...) {{{
    auto generate_rd_combine = [&printer = _printer, &make_rd_name](
                                   auto &f_var_, auto &t_var_,
                                   std::optional<std::string> type_) {
      std::visit(
          ::prtcl::meta::overload{
              [](std::monostate, auto) {},
              [&printer, &type = type_, &make_rd_name](auto f, auto t) {
                std::string field_ident;
                if (!type)
                  field_ident = "g." + f.value;
                else
                  field_ident = "p." + f.value;
                printer.indent() << field_ident << "[0] ";
                namespace rd_tag = ::prtcl::tag::reduce;
                auto name = make_rd_name(f, type);
                if constexpr (rd_tag::plus{} == t)
                  printer << "+= " << name << ";" << '\n';
                else if constexpr (rd_tag::minus{} == t)
                  printer << "-= " << name << ";" << '\n';
                else if constexpr (rd_tag::multiplies{} == t)
                  printer << "*= " << name << ";" << '\n';
                else if constexpr (rd_tag::divides{} == t)
                  printer << "/= " << name << ";" << '\n';
                else if constexpr (rd_tag::min{} == t)
                  printer << " = std::min(" << field_ident << "[0], " << name
                          << ");" << '\n';
                else if constexpr (rd_tag::max{} == t)
                  printer << " = std::max(" << field_ident << "[0], " << name
                          << ");" << '\n';
                else
                  throw "invalid reduce tag";
              }},
          f_var_, t_var_);
    };
    // }}}

    if (reductions.global_fields.size()) {
      _printer << '\n';

      _printer.indent() << "// initialize global reduction variables" << '\n';
      // generate_rd_init {{{
      for (auto both : boost::range::combine(
               reductions.global_fields, reductions.global_reduce_tags))
        generate_rd_init(boost::get<0>(both), boost::get<1>(both), {});
      // }}}
    }

    _printer << '\n';

    // for each selected type
    for (auto type : extractor.types) {
      // loop over particle groups
      _printer.indent() << "for (auto &p : _groups_" << type << ") {" << '\n';
      _printer.increase_indent();

      if (reductions.uniform_fields[type].size()) {
        _printer.indent() << "// initialize uniform reduction variables"
                          << '\n';
        // generate_rd_init {{{
        for (auto both : boost::range::combine(
                 reductions.uniform_fields[type],
                 reductions.uniform_reduce_tags[type]))
          generate_rd_init(boost::get<0>(both), boost::get<1>(both), type);
        // }}}
      }

      _printer << '\n';

      // loop over particle indices
      _printer.indent() << "#pragma omp for" << '\n';
      _printer.indent() << "for (size_t i = 0; i < p._size; ++i) {" << '\n';
      _printer.increase_indent();

      _printer.indent() << "for (auto &pgn : neighbours)" << '\n';
      _printer.increase_indent();
      _printer.indent() << "pgn.clear();" << '\n';
      _printer.decrease_indent();

      _printer << '\n';

      // lazy check if neighbours of this particle have been computed already
      _printer.indent() << "bool has_neighbours = false;" << '\n';
      _printer.indent() << "(void)(has_neighbours);" << '\n';

      // generate code for the selectors
      boost::hana::for_each(
          boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
              expr.elements),
          [this, &type](auto &&e) {
            using namespace boost::hana::literals;
            auto selector = std::forward<decltype(e)>(e).elements[0_c].value();
            if (type == selector.type)
              boost::yap::transform_strict(std::forward<decltype(e)>(e), *this);
          });

      // close loop over particle indices
      _printer.decrease_indent();
      _printer.indent() << "} // i" << '\n';

      if (reductions.uniform_fields[type].size()) {
        _printer << '\n';
        _printer.indent() << "// combine reduction variables" << '\n';
        // generate_rd_combine {{{
        _printer.indent() << "#pragma omp critical" << '\n';
        _printer.indent() << "{" << '\n';
        _printer.increase_indent();

        for (auto both : boost::range::combine(
                 reductions.uniform_fields[type],
                 reductions.uniform_reduce_tags[type]))
          generate_rd_combine(boost::get<0>(both), boost::get<1>(both), type);

        _printer.decrease_indent();
        _printer.indent() << "}" << '\n';
        // }}}
      }

      // close loop over particle groups
      _printer.decrease_indent();
      _printer.indent() << "} // p" << '\n';
    }

    if (reductions.global_fields.size()) {
      _printer << '\n';
      _printer.indent() << "// combine reduction variables" << '\n';
      // generate_rd_combine {{{
      _printer.indent() << "#pragma omp critical" << '\n';
      _printer.indent() << "{" << '\n';
      _printer.increase_indent();

      for (auto both : boost::range::combine(
               reductions.global_fields, reductions.global_reduce_tags))
        generate_rd_combine(boost::get<0>(both), boost::get<1>(both), {});

      _printer.decrease_indent();
      _printer.indent() << "}" << '\n';
      // }}}
    }

    // close the parallel region
    _printer.decrease_indent();
    _printer.indent() << "} // omp parallel" << '\n';

    _printer.decrease_indent();
    _printer.indent() << "} // }}}" << '\n';
  }

  template <typename... Exprs>
  void operator()(call_expr<term<::prtcl::expr::selector>, Exprs...> expr) {
    using namespace boost::hana::literals;
    // generate code for the loop body
    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this, type = expr.elements[0_c].value().type](auto &&e) {
          _printer << '\n';
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer, type},
              neighbour_loop_xform{_printer, type});
        });
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
  void operator()(call_expr<term<::prtcl::expr::section>, Exprs...> expr) {
    using namespace boost::hana::literals;
    _printer.indent() << "template <typename NHood>" << '\n';
    _printer.indent() << "void " << expr.elements[0_c].value().name
                      << "(NHood const &nhood_) {\n";
    _printer.increase_indent();

    _printer.indent() << "(void)(nhood_);" << '\n';
    _printer << '\n';

    _printer.indent() << "auto &g = _global;" << '\n';
    _printer.indent() << "(void)(g);" << '\n';

    boost::hana::for_each(
        boost::hana::slice_c<1, boost::hana::length(expr.elements)>(
            expr.elements),
        [this](auto &&e) {
          _printer << '\n';
          boost::yap::transform_strict(
              std::forward<decltype(e)>(e), statement_xform{_printer},
              particle_loop_xform{_printer});
        });
    _printer.decrease_indent();
    _printer.indent() << "}\n";
  }

public:
  explicit section_xform(cxx_printer printer_) : _printer{printer_} {}

private:
  cxx_printer _printer;
};

// }}}

} // namespace n_openmp

template <typename... Exprs>
void generate_openmp_source(
    std::ostream &stream_, std::string name_, Exprs &&... exprs_) {
  namespace omp = ::prtcl::n_openmp;
  omp::requirements reqs;

  // collect all section expressions
  auto section_exprs = boost::hana::make_tuple(std::forward<Exprs>(exprs_)...);

  boost::hana::for_each(section_exprs, [&reqs](auto expr_) {
    // extract all requirements of the sections on the
    // groups
    omp::extract_requirements(reqs, expr_);
  });

  omp::cxx_printer printer{stream_};

  auto make_field_handler = [](auto &&callable_) {
    return ::prtcl::meta::overload{std::forward<decltype(callable_)>(callable_),
                                   [](std::monostate) {}};
  };

  printer << "#pragma once" << '\n';
  printer << '\n';

  printer << "#include <prtcl/model_base.hpp>" << '\n';
  printer << '\n';
  printer << "#include <string_view>" << '\n';
  printer << "#include <variant>" << '\n';
  printer << "#include <vector>" << '\n';
  printer << '\n';
  printer << "#include <cstddef>" << '\n';
  printer << '\n';
  printer << "#include <omp.h>" << '\n';

  printer << '\n';
  printer << "namespace prtcl::scheme {" << '\n';
  printer << '\n';

  printer.indent() << "template <typename T, size_t N> class " << name_
                   << " : private ::prtcl::model_base {" << '\n';
  printer.increase_indent();

  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();
  printer.indent()
      << "using scalar_type = typename ndfield_ref_t<T>::value_type;" << '\n';
  printer.indent()
      << "using vector_type = typename ndfield_ref_t<T, N>::value_type;"
      << '\n';
  printer.indent()
      << "using matrix_type = typename ndfield_ref_t<T, N, N>::value_type;"
      << '\n';

  printer << '\n';

  // generate: struct global_data { ... }; {{{
  printer.decrease_indent();
  printer.indent() << "private:" << '\n';
  printer.increase_indent();

  printer.indent() << "struct global_data { // {{{" << '\n';
  printer.increase_indent();

  // generate code for the group types
  for (auto f_var : reqs.scheme_fields)
    std::visit(
        make_field_handler([&printer](auto f) {
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
    std::visit(
        make_field_handler([&printer](auto f) {
          printer.indent() << "s_.add(\"" << f.value << "\"_g"
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

  printer.indent() << "void load(scheme_t<T, N> const &s_) {" << '\n';
  printer.increase_indent();

  printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
  for (auto f_var : reqs.scheme_fields)
    std::visit(
        make_field_handler([&printer](auto f) {
          printer.indent() << f.value << " = "
                           << "s_.get(\"" << f.value << "\"_g"
                           << boost::lexical_cast<std::string>(f.type_tag)[0]
                           << "f);" << '\n';
        }),
        f_var);

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  // }}}

  printer.decrease_indent();
  printer.indent() << "};" << '\n';
  printer.indent() << "// }}}" << '\n';
  // }}}

  printer << '\n';

  for (auto [name, fields] : reqs.group_fields) {
    // generate: struct: ..._group_data {{{
    printer.decrease_indent();
    printer.indent() << "private:" << '\n';
    printer.increase_indent();

    printer.indent() << "struct " << name << "_group_data { // {{{" << '\n';
    printer.increase_indent();

    // generate code for the group types
    for (auto f_var : fields)
      std::visit(
          make_field_handler([&printer](auto f) {
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
    printer.indent() << "size_t _index;" << '\n';

    printer << '\n';
    // generate: static: require(...) {{{
    printer.decrease_indent();
    printer.indent() << "public:" << '\n';
    printer.increase_indent();

    printer.indent() << "static void require(group_t<T, N> &g_) {" << '\n';
    printer.increase_indent();

    printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
    for (auto f_var : fields)
      std::visit(
          make_field_handler([&printer](auto f) {
            printer.indent() << "g_.add(\"" << f.value << "\"_"
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

    printer.indent() << "void load(group_t<T, N> const &g_) {" << '\n';
    printer.increase_indent();

    printer.indent() << "using namespace ::prtcl::expr_literals;" << '\n';
    for (auto f_var : fields)
      std::visit(
          make_field_handler([&printer](auto f) {
            printer.indent() << f.value << " = "
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
    printer.indent() << "// }}}" << '\n';
    // }}}

    printer << '\n';
  }

  // generate: member variables {{{
  printer.decrease_indent();
  printer.indent() << "private:" << '\n';
  printer.increase_indent();

  printer.indent() << "size_t _group_count;" << '\n';
  printer.indent() << "global_data _global;" << '\n';

  for (auto [name, fields] : reqs.group_fields) {
    (void)(fields);
    printer.indent() << "std::vector<" << name << "_group_data> _groups_"
                     << name << ";" << '\n';
  }

  printer.indent()
      << "std::vector<std::vector<std::vector<size_t>>> _per_thread_neighbours;"
      << '\n';
  // }}}

  printer << '\n';

  // generate: static: require(...) { ... } {{{

  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "static void require(scheme_t<T, N> &s_) { // {{{"
                   << '\n';
  printer.increase_indent();

  printer.indent() << "global_data::require(s_);" << '\n';
  printer.indent() << '\n';

  printer.indent() << "for (auto &group : s_.groups()) {" << '\n';
  printer.increase_indent();

  for (auto [name, fields] : reqs.group_fields) {
    (void)(fields);

    printer.indent() << "if (group.get_type() == \"" << name << "\")" << '\n';
    printer.increase_indent();
    printer.indent() << name << "_group_data::require(group);" << '\n';
    printer.decrease_indent();
  }

  printer.decrease_indent();
  printer.indent() << "}" << '\n';

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  printer.indent() << "// }}}" << '\n';

  // }}}

  printer << '\n';

  // generate: member: load(...) { ... } {{{

  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "void load(scheme_t<T, N> const &s_) { // {{{" << '\n';
  printer.increase_indent();

  printer.indent() << "_group_count = s_.get_group_count();" << '\n';
  printer << '\n';
  printer.indent() << "_global.load(s_);" << '\n';
  printer << '\n';

  printer.indent() << "for (auto [i, n, group] : s_.enumerate_groups()) {"
                   << '\n';
  printer.increase_indent();

  for (auto [name, fields] : reqs.group_fields) {
    (void)(fields);

    printer.indent() << "if (group.get_type() == \"" << name << "\") {" << '\n';
    printer.increase_indent();

    printer.indent() << "auto &data = _groups_" << name << ".emplace_back();"
                     << '\n';
    printer.indent() << "data.load(group);" << '\n';
    printer.indent() << "data._index = i;" << '\n';

    printer.decrease_indent();
    printer.indent() << "}" << '\n';
  }

  printer.decrease_indent();
  printer.indent() << "}" << '\n';

  printer.decrease_indent();
  printer.indent() << "}" << '\n';

  printer.indent() << "// }}}" << '\n';

  // }}}

  boost::hana::for_each(section_exprs, [&printer](auto &&expr_) {
    printer << '\n';

    printer.decrease_indent();
    printer.indent() << "public:" << '\n';
    printer.increase_indent();

    // generate code for all sections
    boost::yap::transform_strict(
        std::forward<decltype(expr_)>(expr_), omp::section_xform{printer});
  });

  printer << '\n';

  // generate: static: xml() {{{
  printer.decrease_indent();
  printer.indent() << "public:" << '\n';
  printer.increase_indent();

  printer.indent() << "static std::string_view xml() { // {{{" << '\n';
  printer.increase_indent();

  printer.indent() << "return R\"prtcl(" << '\n';
  printer << "<prtcl>" << '\n';

  boost::hana::for_each(section_exprs, [&printer](auto &&expr_) {
    ::prtcl::expr::print(
        printer.stream(), std::forward<decltype(expr_)>(expr_));
  });

  printer << "</prtcl>" << '\n';
  printer << ")prtcl\";" << '\n';

  printer.decrease_indent();
  printer.indent() << "}" << '\n';
  printer.indent() << "// }}}" << '\n';
  // }}}

  printer.decrease_indent();
  printer.indent() << "};" << '\n';

  printer << '\n';
  printer << "} // namespace prtcl::scheme" << '\n';
}

} // namespace prtcl

 */
