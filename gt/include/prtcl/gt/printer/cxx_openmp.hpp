#pragma once

#include "../bits/cxx_escape_double_quotes.hpp"
#include "../bits/cxx_inline_pragma.hpp"
#include "../bits/printer_crtp.hpp"

#include "../ast.hpp"

#include "../misc/alias_to_field_map.hpp"
#include "../misc/alias_to_particle_selector_map.hpp"
#include "../misc/particle_selector_to_fields_map.hpp"
#include "../misc/reduction_set.hpp"

#include <set>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include <boost/lexical_cast.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::gt::printer {

struct cxx_openmp : public printer_crtp<printer::cxx_openmp> {
public:
  using base_type::base_type;
  using base_type::operator();

protected:
  void cxx_access_modifier(std::string name_) {
    decrease_indent();
    outi() << name_ << ':' << nl;
    increase_indent();
  }

  static std::string qualified_nd_dtype(ast::basic_type type) {
    std::ostringstream ss;
    ss << "nd_dtype::" << type;
    return ss.str();
  }

  static std::string nd_template_args(ast::nd_type nd_type) {
    std::ostringstream ss;
    ss << qualified_nd_dtype(nd_type.type)
       << (nd_type.shape.empty() ? "" : ", ")
       << join(nd_type.shape, ", ", "", [](auto n) -> std::string {
            return n == 0 ? "N" : boost::lexical_cast<std::string>(n);
          });
    return ss.str();
  }

  static ast::math::constant
  rd_initializer(ast::stmt::reduce_op op_, ast::nd_type nd_type) {
    auto nd_args = nd_template_args(nd_type);
    switch (op_) {
    case ast::stmt::reduce_op::add_assign:
    case ast::stmt::reduce_op::sub_assign:
      return ast::math::constant{"zeros", nd_type};
      break;
    case ast::stmt::reduce_op::mul_assign:
    case ast::stmt::reduce_op::div_assign:
      return ast::math::constant{"ones", nd_type};
      break;
    case ast::stmt::reduce_op::max_assign:
      return ast::math::constant{"negative_infinity", nd_type};
      break;
    case ast::stmt::reduce_op::min_assign:
      return ast::math::constant{"positive_infinity", nd_type};
      break;
    }
    throw "internal error: unexpected reduce_op";
  }

  static auto only_fields(ast::storage_qualifier storage_) {
    return boost::adaptors::filtered(
        [storage_](auto const &field) { return field.storage == storage_; });
  }

private:
  static constexpr char const *_hpp_header = R"HEADER(
#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

)HEADER";

  static constexpr char const *_hpp_footer = R"FOOTER(

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
  )FOOTER";

public:
  //! Print a constant.
  void operator()(ast::math::constant const &arg_) {
    out() << "c::template " << arg_.constant_name << '<'
          << nd_template_args(arg_.constant_type) << '>' << "()";
  }

  //! Print a literal. TODO: debug format
  void operator()(ast::math::literal const &arg_) {
    out() << "l::template narray<" << nd_template_args(arg_.type) << ">({"
          << arg_.value << "})";
  }

  //! Print access to a field (eg. x[f]).
  void operator()(ast::math::field_access const &arg_) {
    // {{{ implementation
    auto field = alias_to_field.search(arg_.field_name).value();

    char group, index;
    if (field.storage == ast::storage_qualifier::global) {
      group = 'g';
      index = '0';
    } else {
      if (cur_particle and arg_.index_name == cur_particle->index_name) {
        group = 'p';
        index = 'i';
      }
      if (cur_neighbor and arg_.index_name == cur_neighbor->index_name) {
        group = 'n';
        index = 'j';
      }
      if (field.storage == ast::storage_qualifier::uniform)
        index = '0';
    }

    out() << group << '.' << field.field_name << '[' << index << ']';
    // }}}
  }

  //! Print a function call.
  void operator()(ast::math::function_call const &arg_) {
    out() << "o::" << arg_.function_name << '(' << sep(arg_.arguments, ", ")
          << ')';
  }

  //! Print the arithmetic n-ary operation.
  void operator()(ast::math::arithmetic_nary const &arg_) {
    // {{{ implementation
    if (not arg_.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << '(';
    // print the first operand
    (*this)(arg_.first_operand);
    for (auto const &anr : arg_.right_hand_sides) {
      // print the operator
      out() << ' ' << anr.op << ' ';
      // print the operand
      (*this)(anr.rhs);
    }
    if (not arg_.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << ')';
    // }}}
  }

  void operator()(ast::stmt::reduce const &arg_, bool combine_ = false) {
    // {{{ implemenetation
    auto field = alias_to_field.search(arg_.field_name).value();
    if ((cur_particle and cur_neighbor) or
        (cur_particle and not cur_neighbor)) {
      if (field.storage == ast::storage_qualifier::varying)
        throw "internal error: cannot reduce into varying fields";
    } else
      throw "internal error: reduce not placed in a particle or "
            "particle/neighbor loop";

    auto lhs = ast::math::field_access{arg_.field_name, arg_.index_name};

    std::string op_str, suffix;
    switch (arg_.op) {
    case ast::stmt::reduce_op::max_assign:
      op_str = "= o::max(";
      suffix = ")";
      break;
    case ast::stmt::reduce_op::min_assign:
      op_str = "= o::min(";
      suffix = ")";
      break;
    default:
      op_str = boost::lexical_cast<std::string>(arg_.op) + " ";
      break;
    }

    outi();

    if (combine_) {
      (*this)(lhs);
      out() << ' ' << op_str << "rd_" << field.field_name << suffix;
    } else {
      out() << "rd_" << field.field_name << ' ' << op_str;

      switch (arg_.op) {
      case ast::stmt::reduce_op::max_assign:
      case ast::stmt::reduce_op::min_assign:
        out() << "rd_" << field.field_name;
        out() << ", ";
        break;
      default:
        break;
      }

      (*this)(arg_.expression);
      out() << suffix;
    }

    out() << ';' << nl;
    // }}}
  }

  void operator()(ast::stmt::compute const &arg_) {
    // {{{ implemenetation
    auto field = alias_to_field.search(arg_.field_name).value();
    if (field.storage != ast::storage_qualifier::varying)
      throw "internal error: can only compute into varying fields";

    auto lhs = ast::math::field_access{arg_.field_name, arg_.index_name};

    std::string op_str;
    switch (arg_.op) {
    case ast::stmt::compute_op::max_assign:
      op_str = "= o::max(";
      break;
    case ast::stmt::compute_op::min_assign:
      op_str = "= o::min(";
      break;
    default:
      break;
    }

    outi();
    (*this)(lhs);
    switch (arg_.op) {
    case ast::stmt::compute_op::max_assign:
    case ast::stmt::compute_op::min_assign:
      out() << ' ' << op_str;
      (*this)(lhs);
      out() << ", ";
      break;
    default:
      out() << ' ' << arg_.op << ' ';
      break;
    }
    (*this)(arg_.expression);

    switch (arg_.op) {
    case ast::stmt::compute_op::max_assign:
    case ast::stmt::compute_op::min_assign:
      out() << ")";
      break;
    default:
      break;
    }

    out() << ';' << nl;
    // }}}
  }

  void operator()(ast::stmt::foreach_neighbor const &arg_) {
    // {{{ implementation
    if (cur_neighbor)
      throw "internal error: neighbor loop inside of neighbor loop";
    else
      cur_neighbor = loop_type{arg_.selector_name, arg_.neighbor_index_name};

    outi() << '{' << "// foreach " << arg_.selector_name << " neighbor "
           << arg_.neighbor_index_name << nl;
    {
      increase_indent();

      outi() << "PRTCL_RT_LOG_TRACE_SCOPED(\"foreach_neighbor\", \"n="
             << arg_.selector_name << "\");" << nl;
      out() << nl;

      outi() << "for (auto &n : _data.by_group_type." << arg_.selector_name
             << ") {" << nl;
      {
        increase_indent();

        outi() << "PRTCL_RT_LOG_TRACE_PLOT_NUMBER(\"neighbor count\", "
                  "static_cast<int64_t>(neighbors[n._index].size()));"
               << nl;
        out() << nl;

        outi() << "for (auto const j : neighbors[n._index]) {" << nl;
        {
          increase_indent();

          // iterate over all child nodes
          bool first_iteration = true;
          for (auto const &statement : arg_.statements) {
            if (not first_iteration)
              out() << nl;
            else
              first_iteration = false;
            (*this)(statement);
          }

          decrease_indent();
        }
        outi() << "}" << nl;

        decrease_indent();
      }
      outi() << "}" << nl;

      decrease_indent();
    }
    outi() << '}' << nl;

    if (not cur_neighbor)
      throw "internal error: neighbor loop was already unset";
    else
      cur_neighbor.reset();
    // }}}
  }

  void operator()(ast::stmt::foreach_particle const &arg_) {
    // {{{ implementation
    if (cur_particle)
      throw "internal error: particle loop inside of particle loop";
    else
      cur_particle = loop_type{arg_.selector_name, arg_.particle_index_name};

    auto reductions = make_reduction_set(arg_);

    bool const has_foreach_neighbor_offspring =
        boost::range::count_if(arg_.statements, [](auto const &statement_) {
          return std::holds_alternative<ast::stmt::foreach_neighbor>(
              statement_);
        });

    outi() << "PRTCL_RT_LOG_TRACE_SCOPED(\"foreach_particle\", \"p="
           << arg_.selector_name << "\");" << nl;
    out() << nl;

    outi() << cxx_inline_pragma("omp single") << " {" << nl;
    {
      increase_indent();

      outi() << "auto const thread_count = "
                "static_cast<size_t>(omp_get_num_threads());"
             << nl;

      // always resize the vector containing thread local data (eg. global
      // reductions which are independent of neighbor loops)
      outi() << "_per_thread.resize(thread_count);" << nl;

      decrease_indent();
    }
    outi() << "} // pragma omp single" << nl;

    out() << nl;
    outi() << "auto const thread_index = "
              "static_cast<size_t>(omp_get_thread_num());"
           << nl;
    out() << nl;

    if (has_foreach_neighbor_offspring) {
      outi() << "// select and resize the neighbor storage for the current "
                "thread"
             << nl;
      outi() << "auto &neighbors = _per_thread[thread_index].neighbors;" << nl;
      outi() << "neighbors.resize(_group_count);" << nl;
      out() << nl;
      outi() << "for (auto &pgn : neighbors)" << nl;
      {
        increase_indent();
        outi() << "pgn.reserve(100);" << nl;
        decrease_indent();
      }
      out() << nl;
    }

    if (not reductions.empty()) {
      for (auto const &reduce : reductions.items()) {
        auto field = alias_to_field.search(reduce.field_name).value();

        outi() << "// select and initialize this threads reduction variable"
               << nl;
        outi() << "auto &rd_" << field.field_name
               << " = _per_thread[thread_index].rd_" << field.field_name << ";"
               << nl;
        outi() << "rd_" << field.field_name << " = ";
        (*this)(rd_initializer(reduce.op, field.field_type));
        out() << ";" << nl;
        out() << nl;
      }
    }

    outi() << "for (auto &p : _data.by_group_type." << arg_.selector_name
           << ") {" << nl;
    {
      increase_indent();

      out() << "#pragma omp for" << nl;
      outi() << "for (size_t i = 0; i < p._count; ++i) {" << nl;
      {
        increase_indent();

        if (has_foreach_neighbor_offspring) {
          outi() << "// clean up the neighbor storage" << nl;
          outi() << "for (auto &pgn : neighbors)" << nl;
          {
            increase_indent();
            outi() << "pgn.clear();" << nl;
            decrease_indent();
          }
          out() << nl;
          outi() << "// find all neighbors of (p, i)" << nl;
          outi() << "nhood_.neighbors(p._index, i, [&neighbors](auto n_index, "
                    "auto j) {"
                 << nl;
          {
            increase_indent();
            outi() << "neighbors[n_index].push_back(j);" << nl;
            decrease_indent();
          }
          outi() << "});" << nl;
        } else
          outi() << "// no neighbours neccessary" << nl;

        // iterate over all child nodes
        for (auto const &statement : arg_.statements) {
          out() << nl;
          (*this)(statement);
        }

        decrease_indent();
      }
      outi() << "}" << nl;

      decrease_indent();
    }
    outi() << "}" << nl;

    if (not reductions.empty()) {
      out() << nl;
      outi() << cxx_inline_pragma("omp critical") << " {" << nl;
      {
        increase_indent();

        outi() << "PRTCL_RT_LOG_TRACE_SCOPED(\"reduction\");" << nl;
        out() << nl;

        outi() << "// combine all reduction variables" << nl;
        for (auto const &reduce : reductions.items()) {
          out() << nl;
          (*this)(reduce, true);
        }

        decrease_indent();
      }
      outi() << "} // pragma omp critical" << nl;
    }

    if (not cur_particle)
      throw "internal error: particle loop was already unset";
    else
      cur_particle.reset();
    // }}}
  }

  void operator()(ast::stmt::procedure const &arg_) {
    out() << nl;
    cxx_access_modifier("public");
    outi() << "template <typename NHood_>" << nl;
    outi() << "void " << arg_.procedure_name << "(NHood_ const &nhood_) {"
           << nl;
    { // {{{ implementation
      increase_indent();

      outi() << "// alias for the global data" << nl;
      outi() << "auto &g = _data.global;" << nl;
      out() << nl;

      outi() << "// alias for the math_policy member types" << nl;
      outi() << "using l = typename math_policy::literals;" << nl;
      outi() << "using c = typename math_policy::constants;" << nl;
      outi() << "using o = typename math_policy::operations;" << nl;

      bool in_parallel_region = false;
      auto begin_parallel_region = [this, &in_parallel_region]() {
        if (not in_parallel_region) {
          out() << nl;
          outi() << cxx_inline_pragma("omp parallel") << " {" << nl;
          increase_indent();
          in_parallel_region = true;
        }
      };

      auto close_parallel_region = [this, &in_parallel_region]() {
        if (in_parallel_region) {
          in_parallel_region = false;
          decrease_indent();
          outi() << "} // pragma omp parallel" << nl;
        }
      };

      // iterate over all child nodes
      for (auto const &statement : arg_.statements) {
        // conditionally begin and close the parallel region
        if (std::holds_alternative<ast::stmt::foreach_particle>(statement))
          begin_parallel_region();
        else {
          close_parallel_region();
          out() << nl;
        }
        (*this)(statement);
      }
      // if we're still in a parallel region, close it
      if (in_parallel_region)
        close_parallel_region();

      decrease_indent();
    } // }}}
    outi() << "}" << nl;
  }

  void operator()(ast::init::field const &) {
    // ignore
  }

  void operator()(ast::init::particle_selector const &) {
    // ignore
  }

  void operator()(ast::stmt::let const &) {
    // ignore
  }

  void operator()(ast::prtcl_source_file const &arg_) {
    // {{{ implementation
    alias_to_field = make_alias_to_field_map(arg_);
    alias_to_particle_selector = make_alias_to_particle_selector_map(arg_);
    particle_selector_to_fields = make_particle_selector_to_fields_map(arg_);
    auto reductions = make_reduction_set(arg_);

    out() << _hpp_header;

    outi() << join(_namespaces, " ", "", [](auto ns_) {
      return "namespace " + ns_ + " {";
    }) << nl;

    out() << nl;

    outi() << "template <" << nl;
    {
      increase_indent();
      outi() << "typename ModelPolicy_" << nl;
      decrease_indent();
    }
    outi() << ">" << nl;
    outi() << "class " << _name << " {" << nl;
    {
      increase_indent();

      cxx_access_modifier("public");
      // {{{ type aliases
      outi() << "using model_policy = ModelPolicy_;" << nl;
      outi() << "using type_policy = typename model_policy::type_policy;" << nl;
      outi() << "using math_policy = typename model_policy::math_policy;" << nl;
      outi() << "using data_policy = typename model_policy::data_policy;" << nl;
      out() << nl;
      outi() << "using nd_dtype = prtcl::rt::nd_dtype;" << nl;
      out() << nl;
      outi() << "template <nd_dtype DType_> using dtype_t = typename "
                "type_policy::template dtype_t<DType_>;"
             << nl;
      outi() << "template <nd_dtype DType_, size_t ...Ns_> using nd_dtype_t = "
                "typename math_policy::template nd_dtype_t<DType_, Ns_...>;"
             << nl;
      outi() << "template <nd_dtype DType_, size_t ...Ns_> using "
                "nd_dtype_data_ref_t = typename data_policy::template "
                "nd_dtype_data_ref_t<DType_, Ns_...>;"
             << nl;
      out() << nl;
      outi() << "static constexpr size_t N = model_policy::dimensionality;"
             << nl;
      out() << nl;
      outi() << "using model_type = prtcl::rt::basic_model<model_policy>;"
             << nl;
      outi() << "using group_type = prtcl::rt::basic_group<model_policy>;"
             << nl;
      out() << nl;
      // }}}

      auto global_field_aliases_opt = particle_selector_to_fields.search("");
      if (global_field_aliases_opt) {
        auto const &global_field_aliases = global_field_aliases_opt.value();

        cxx_access_modifier("private");
        outi() << "struct global_data {" << nl;
        { // {{{
          increase_indent();

          for (auto const &alias : global_field_aliases) {
            auto field = alias_to_field.search(alias).value();
            outi() << "nd_dtype_data_ref_t<"
                   << nd_template_args(field.field_type) << "> "
                   << field.field_name << ";" << nl;
          }

          out() << nl;

          outi() << "static void _require(model_type &m_) {" << nl;
          { // {{{
            increase_indent();

            for (auto const &alias : global_field_aliases) {
              auto field = alias_to_field.search(alias).value();
              outi() << "m_.template add_global<"
                     << nd_template_args(field.field_type) << ">(\""
                     << field.field_name << "\");" << nl;
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          out() << nl;

          outi() << "void _load(model_type const &m_) {" << nl;
          { // {{{
            increase_indent();

            for (auto const &alias : global_field_aliases) {
              auto field = alias_to_field.search(alias).value();
              outi() << field.field_name << " = "
                     << "m_.template get_global<"
                     << nd_template_args(field.field_type) << ">(\""
                     << field.field_name << "\");" << nl;
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          decrease_indent();
        } // }}}
        outi() << "};" << nl;
        out() << nl;
      }

      for (auto [selector, aliases] : particle_selector_to_fields.items()) {
        // skip the global fields
        if (selector == "")
          continue;
        // TODO: have a better way to store the implicit global selector

        cxx_access_modifier("private");
        outi() << "struct " << selector << "_data {" << nl;
        { // {{{
          increase_indent();

          outi() << "// particle count of the selected group" << nl;
          outi() << "size_t _count;" << nl;
          outi() << "// index of the selected group" << nl;
          outi() << "size_t _index;" << nl;
          out() << nl;

          auto all_fields = aliases | alias_to_field.resolver();
          if (not(all_fields | only_fields(ast::storage_qualifier::global))
                     .empty())
            throw "internal error: global fields used with index from particle "
                  "loop";
          auto uniform_fields =
              all_fields | only_fields(ast::storage_qualifier::uniform);
          auto varying_fields =
              all_fields | only_fields(ast::storage_qualifier::varying);

          // {{{ members
          if (not uniform_fields.empty()) {
            outi() << "// uniform fields" << nl;
            for (auto const &field : uniform_fields) {
              outi() << "nd_dtype_data_ref_t<"
                     << nd_template_args(field.field_type) << "> "
                     << field.field_name << ";" << nl;
            }
            out() << nl;
          }

          if (not varying_fields.empty()) {
            outi() << "// varying fields" << nl;
            for (auto const &field : varying_fields) {
              outi() << "nd_dtype_data_ref_t<"
                     << nd_template_args(field.field_type) << "> "
                     << field.field_name << ";" << nl;
            }
            out() << nl;
          }
          // }}}

          outi() << "static void _require(group_type &g_) {" << nl;
          { // {{{
            increase_indent();

            if (not uniform_fields.empty()) {
              outi() << "// uniform fields" << nl;
              for (auto const &field : uniform_fields) {
                outi() << "g_.template add_uniform<"
                       << nd_template_args(field.field_type) << ">(\""
                       << field.field_name << "\");" << nl;
              }
            }

            if (not uniform_fields.empty() and not varying_fields.empty())
              out() << nl;

            if (not varying_fields.empty()) {
              outi() << "// varying fields" << nl;
              for (auto const &field : varying_fields) {
                outi() << "g_.template add_varying<"
                       << nd_template_args(field.field_type) << ">(\""
                       << field.field_name << "\");" << nl;
              }
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          out() << nl;
          outi() << "void _load(group_type const &g_) {" << nl;
          { // {{{
            increase_indent();

            outi() << "_count = g_.size();" << nl;

            if (not uniform_fields.empty()) {
              out() << nl;
              outi() << "// uniform fields" << nl;
              for (auto const &field : uniform_fields) {
                outi() << field.field_name << " = "
                       << "g_.template get_uniform<"
                       << nd_template_args(field.field_type) << ">(\""
                       << field.field_name << "\");" << nl;
              }
            }

            if (not varying_fields.empty()) {
              out() << nl;
              outi() << "// varying fields" << nl;
              for (auto const &field : varying_fields) {
                outi() << field.field_name << " = "
                       << "g_.template get_varying<"
                       << nd_template_args(field.field_type) << ">(\""
                       << field.field_name << "\");" << nl;
              }
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          decrease_indent();
        } // }}}
        outi() << "};" << nl;
        out() << nl;
      }

      cxx_access_modifier("public");
      outi() << "static void require(model_type &m_) {" << nl;
      { // {{{
        increase_indent();

        outi() << "global_data::_require(m_);" << nl;
        outi() << nl;
        outi() << "for (auto &group : m_.groups()) {" << nl;
        {
          increase_indent();

          for (auto const &[selector_alias, aliases] :
               particle_selector_to_fields.items()) {
            // skip the global fields
            if (selector_alias == "")
              continue;
            // TODO: have a better way to store the implicit global selector

            auto selector =
                alias_to_particle_selector.search(selector_alias).value();

            outi() << "if (("
                   << join(
                          selector.type_disjunction, " or ", "true",
                          [](auto type_name) {
                            return "group.get_type() == \"" + type_name + "\"";
                          })
                   << ") and ("
                   << join(
                          selector.tag_conjunction, " and ", "true",
                          [](auto tag_name) {
                            return "group.has_tag(\"" + tag_name + "\")";
                          })
                   << "))"
                   << " {" << nl;
            // TODO: test tags
            {
              increase_indent();
              outi() << selector_alias << "_data::_require(group);" << nl;
              decrease_indent();
            }
            outi() << "}" << nl;
          }

          decrease_indent();
        }
        outi() << "}" << nl;

        decrease_indent();
      } // }}}
      outi() << "}" << nl;

      outi() << nl;

      cxx_access_modifier("public");
      outi() << "void load(model_type &m_) {" << nl;
      { // {{{
        increase_indent();

        outi() << "_group_count = m_.groups().size();" << nl;
        out() << nl;
        outi() << "_data.global._load(m_);" << nl;
        out() << nl;
        for (auto const &[selector_alias, aliases] :
             particle_selector_to_fields.items()) {
          // skip the global fields
          if (selector_alias == "")
            continue;
          outi() << "_data.by_group_type." << selector_alias << ".clear();"
                 << nl;
        }
        out() << nl;
        outi() << "auto groups = m_.groups();" << nl;
        outi() << "for (size_t i = 0; i < groups.size(); ++i) {" << nl;
        {
          increase_indent();

          outi() << "auto &group = groups[static_cast<typename "
                    "decltype(groups)::difference_type>(i)];"
                 << nl;

          for (auto const &[selector_alias, aliases] :
               particle_selector_to_fields.items()) {
            // skip the global fields
            if (selector_alias == "")
              continue;
            // TODO: have a better way to store the implicit global selector

            auto selector =
                alias_to_particle_selector.search(selector_alias).value();

            out() << nl;
            outi() << "if (("
                   << join(
                          selector.type_disjunction, " or ", "true",
                          [](auto type_name) {
                            return "group.get_type() == \"" + type_name + "\"";
                          })
                   << ") and ("
                   << join(
                          selector.tag_conjunction, " and ", "true",
                          [](auto tag_name) {
                            return "group.has_tag(\"" + tag_name + "\")";
                          })
                   << "))"
                   << " {" << nl;
            // TODO: test tags
            {
              increase_indent();

              outi() << "auto &data = _data.by_group_type." << selector_alias
                     << ".emplace_back();" << nl;
              outi() << "data._load(group);" << nl;
              outi() << "data._index = i;" << nl;

              decrease_indent();
            }
            outi() << "}" << nl;
          }

          decrease_indent();
        }
        outi() << "}" << nl;

        decrease_indent();
      } // }}}
      outi() << "}" << nl;

      outi() << nl;

      cxx_access_modifier("private");
      outi() << "struct {" << nl;
      { // {{{
        increase_indent();

        outi() << "global_data global;" << nl;
        outi() << "struct {" << nl;
        {
          increase_indent();

          for (auto const &[selector_alias, aliases] :
               particle_selector_to_fields.items()) {
            // skip the global fields
            if (selector_alias == "")
              continue;
            // TODO: have a better way to store the implicit global selector

            outi() << "std::vector<" << selector_alias << "_data> "
                   << selector_alias << ";" << nl;
          }

          decrease_indent();
        }
        outi() << "} by_group_type;" << nl;

        decrease_indent();
      } // }}}
      outi() << "} _data;" << nl;

      out() << nl;

      outi() << "struct per_thread_type {" << nl;
      { // {{{
        increase_indent();

        outi() << "std::vector<std::vector<size_t>> "
                  "neighbors;"
               << nl;
        out() << nl;
        outi() << "// reductions" << nl;
        for (auto const &reduce : reductions.items()) {
          auto field = alias_to_field.search(reduce.field_name).value();

          outi() << "nd_dtype_t<" << nd_template_args(field.field_type)
                 << "> rd_" << field.field_name << ';' << nl;
        }

        decrease_indent();
      } // }}}
      outi() << "};" << nl;

      out() << nl;

      outi() << "std::vector<per_thread_type> _per_thread;" << nl;
      outi() << "size_t _group_count;" << nl;

      // iterate over all child nodes
      for (auto const &statement : arg_.statements) {
        (*this)(statement);
      }

      decrease_indent();
    }
    outi() << "};" << nl;

    out() << nl;

    outi() << join(
                  _namespaces | boost::adaptors::reversed, " ", "",
                  [](auto ns_) { return "} /* namespace " + ns_ + "*/"; })
           << nl;

    out() << _hpp_footer;

    // TODO: remove
    // outi() << "} // namespace prtcl::schemes" << nl;
    // out() << nl;
    // outi() << "#if defined(__GNUG__)" << nl;
    // outi() << "#pragma GCC diagnostic pop" << nl;
    // outi() << "#endif" << nl;
    // }}}
  }

public:
  template <typename Range_>
  cxx_openmp(std::ostream &stream_, std::string name_, Range_ namespaces_)
      : base_type(stream_), _name{name_}, _namespaces{boost::begin(namespaces_),
                                                      boost::end(namespaces_)} {
  }

private:
  std::string _name;
  std::vector<std::string> _namespaces;

private:
  alias_to_field_map alias_to_field;
  alias_to_particle_selector_map alias_to_particle_selector;
  particle_selector_to_fields_map particle_selector_to_fields;

  struct loop_type {
    std::string selector_name;
    std::string index_name;
  };
  std::optional<loop_type> cur_particle, cur_neighbor;
};

} // namespace prtcl::gt::printer
