#pragma once

#include "../bits/cxx_escape_double_quotes.hpp"
#include "../bits/cxx_inline_pragma.hpp"
#include "../bits/printer_crtp.hpp"
#include "prtcl/gt/bits/ast_define.hpp"

#include <prtcl/core/range.hpp>

#include <prtcl/gt/ast.hpp>

#include <prtcl/gt/misc/find_global_fields.hpp>
#include <prtcl/gt/misc/find_groups.hpp>
#include <prtcl/gt/misc/find_reductions.hpp>

//#include "../misc/alias_to_field_map.hpp"
//#include "../misc/alias_to_particle_selector_map.hpp"
//#include "../misc/particle_selector_to_fields_map.hpp"
//#include "../misc/reduction_set.hpp"

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

  static std::string qualified_dtype_enum(ast::dtype type) {
    std::ostringstream ss;
    switch (type) {
    case ast::dtype::real:
    case ast::dtype::integer:
    case ast::dtype::boolean:
      ss << "dtype::" << to_string(type);
      break;
    default:
      throw ast_printer_error{};
    }
    return ss.str();
  }

  static std::string ndtype_template_args(ast::ndtype type) {
    std::ostringstream ss;
    ss << qualified_dtype_enum(type.type);
    if (not type.shape.empty()) {
      ss << ", ";
      ss << join(type.shape, ", ", "", [](auto n) -> std::string {
        return n == 0 ? "N" : boost::lexical_cast<std::string>(n);
      });
    }
    return ss.str();
  }

  static ast::n_math::operation
  rd_initializer(ast::assign_op op_, ast::ndtype type) {
    switch (op_) {
    case ast::op_add_assign:
    case ast::op_sub_assign:
      return ast::n_math::operation{"zeros", type};
      break;
    case ast::op_mul_assign:
    case ast::op_div_assign:
      return ast::n_math::operation{"ones", type};
      break;
    case ast::op_max_assign:
      return ast::n_math::operation{"negative_infinity", type};
      break;
    case ast::op_min_assign:
      return ast::n_math::operation{"positive_infinity", type};
      break;
    default:
      throw ast_printer_error{};
    }
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
  void operator()(ast::assign_op op) {
    // {{{ implementation
    switch (op) {
    case ast::op_assign:
      break;
    case ast::op_add_assign:
      out() << '+';
      break;
    case ast::op_sub_assign:
      out() << '-';
      break;
    case ast::op_mul_assign:
      out() << '*';
      break;
    case ast::op_div_assign:
      out() << '/';
      break;
    default:
      throw ast_printer_error{};
    }
    out() << '=';
    // }}}
  }

  void operator()(ast::unary_arithmetic_op op) {
    // {{{ implementation
    switch (op) {
    case ast::op_neg:
      out() << '-';
      break;
    default:
      throw ast_printer_error{};
    }
    // }}}
  }

  void operator()(ast::multi_arithmetic_op op) {
    // {{{ implementation
    switch (op) {
    case ast::op_add:
      out() << '+';
      break;
    case ast::op_sub:
      out() << '-';
      break;
    case ast::op_mul:
      out() << '*';
      break;
    case ast::op_div:
      out() << '/';
      break;
    default:
      throw ast_printer_error{};
    }
    // }}}
  }

public:
  //! Print a literal. TODO: debug format
  void operator()(ast::n_math::literal const &node) {
    out() << "o::template narray<" << ndtype_template_args(node.type) << ">({"
          << node.value << "})";
  }

  //! Print a function call.
  void operator()(ast::n_math::operation const &node) {
    // {{{ implementation
    out() << "o::";
    if (node.type.has_value()) {
      out() << "template ";
    }
    out() << node.name;
    if (node.type.has_value()) {
      out() << '<' << ndtype_template_args(node.type.value()) << '>';
    }
    out() << '(' << sep(node.arguments, ", ") << ')';
    // }}}
  }

  //! Print access to a field (eg. x[f]).
  void operator()(ast::n_math::field_access const &node) {
    // {{{ implementation
    if (node.index.has_value()) {
      if (cur_particle and node.index.value() == cur_particle->index_name) {
        out() << "p." << node.field;
        auto &g = groups.groups_for_name(cur_particle->selector_name);
        if (g.uniform_fields.has_alias(node.field))
          out() << "[0]";
        else if (g.varying_fields.has_alias(node.field))
          out() << "[i]";
        else
          throw ast_printer_error{};
      } else if (
          cur_neighbor and node.index.value() == cur_neighbor->index_name) {
        out() << "n." << node.field;
        auto &g = groups.groups_for_name(cur_neighbor->selector_name);
        if (g.uniform_fields.has_alias(node.field))
          out() << "[0]";
        else if (g.varying_fields.has_alias(node.field))
          out() << "[j]";
        else
          throw ast_printer_error{};
      } else {
        throw ast_printer_error{};
      }
    } else {
      if (global.has_alias(node.field)) {
        out() << "g." << node.field << "[0]";
      } else {
        out() << "l_" << node.field;
      }
    }
    // }}}
  }

  //! Print the unary operation.
  void operator()(ast::n_math::unary_arithmetic const &node) {
    (*this)(node.op);
    (*this)(node.operand);
  }

  //! Print the arithmetic n-ary operation.
  void operator()(ast::n_math::multi_arithmetic const &node) {
    // {{{ implementation
    if (not node.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << '(';

    // print the first operand
    (*this)(node.operand);

    for (auto const &rhs : node.right_hand_sides) {
      // print the operator
      out() << ' ';
      (*this)(rhs.op);
      out() << ' ';
      // print the operand
      (*this)(rhs.operand);
    }

    if (not node.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << ')';
    // }}}
  }

  void operator()(ast::n_scheme::local const &node) {
    // {{{ implemenetation
    // format the variable declaration
    outi() << "ndtype_t<" << ndtype_template_args(node.type) << "> l_"
           << node.name << " = ";
    // format the initialization
    (*this)(node.math);
    // end the statement
    out() << ';' << nl;
    // }}}
  }

  void operator()(ast::n_scheme::compute const &node) {
    // {{{ implemenetation
    outi();
    (*this)(node.left_hand_side);
    out() << ' ';

    switch (node.op) {
    case ast::op_max_assign:
      out() << "= o::max( ";
      (*this)(node.left_hand_side);
      out() << ", ";
      break;
    case ast::op_min_assign:
      out() << "= o::min( ";
      (*this)(node.left_hand_side);
      out() << ", ";
      break;
    default:
      (*this)(node.op);
      out() << ' ';
      break;
    }

    (*this)(node.math);

    switch (node.op) {
    case ast::op_max_assign:
    case ast::op_min_assign:
      out() << " )";
      break;
    default:
      break;
    }

    out() << ';' << nl;
    // }}}
  }

  void operator()(ast::n_scheme::reduce const &node) {
    // {{{ implemenetation
    auto &lhs = node.left_hand_side;

    std::string var;
    if (lhs.index.has_value()) {
      var += "urd_";
      if (cur_particle and cur_particle->index_name == lhs.index.value())
        var += cur_particle->selector_name;
      else if (cur_neighbor and cur_neighbor->index_name == lhs.index.value())
        var += cur_neighbor->selector_name;
      else
        throw ast_printer_error{};
    } else {
      var += "grd_" + lhs.field;
    }

    outi() << var << ' ';

    switch (node.op) {
    case ast::op_max_assign:
      out() << "= o::max( " << var << ", ";
      break;
    case ast::op_min_assign:
      out() << "= o::min( " << var << ", ";
      break;
    default:
      (*this)(node.op);
      out() << ' ';
      break;
    }

    (*this)(node.math);

    switch (node.op) {
    case ast::op_max_assign:
    case ast::op_min_assign:
      out() << " )";
      break;
    default:
      break;
    }

    out() << ';' << nl;
    // }}}
  }

  void operator()(ast::n_scheme::foreach_neighbor const &loop) {
    // {{{ implementation
    if (cur_neighbor)
      throw "internal error: neighbor loop inside of neighbor loop";
    else
      cur_neighbor = loop_type{loop.group, loop.index};

    outi() << "{ // foreach " << loop.group << " neighbor " << loop.index << nl;
    {
      increase_indent();

#if defined(PRTCL_GT_CXX_OPENMP_TRACE_FOREACH_NEIGHBOR)
      outi() << "PRTCL_RT_LOG_TRACE_SCOPED(\"foreach_neighbor\", \"n="
             << loop.group << "\");" << nl;
      out() << nl;
#endif // defined(PRTCL_GT_CXX_OPENMP_TRACE_FOREACH_NEIGHBOR)

      outi() << "for (auto &n : _data.groups." << loop.group << ") {" << nl;
      {
        increase_indent();

#if defined(PRTCL_GT_CXX_OPENMP_TRACE_NEIGHBOR_COUNT)
        outi() << "PRTCL_RT_LOG_TRACE_PLOT_NUMBER(\"neighbor count\", "
                  "static_cast<int64_t>(neighbors[n._index].size()));"
               << nl;
        out() << nl;
#endif // defined(PRTCL_GT_CXX_OPENMP_TRACE_NEIGHBOR_COUNT)

        outi() << "for (auto const j : neighbors[n._index]) {" << nl;
        {
          increase_indent();

          // iterate over all child nodes
          bool first_iteration = true;
          for (auto const &statement : loop.statements) {
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
    outi() << "} // foreach " << loop.group << " neighbor " << loop.index << nl;

    if (not cur_neighbor)
      throw "internal error: neighbor loop was already unset";
    else
      cur_neighbor.reset();
    // }}}
  }

  void operator()(ast::n_scheme::foreach_particle const &loop) {
    // {{{ implementation
    if (cur_particle)
      throw "internal error: particle loop inside of particle loop";
    else
      cur_particle = loop_type{loop.group, loop.index};

    auto reductions = find_reductions(loop);

    bool const has_foreach_neighbor_offspring =
        core::ranges::count_if(loop.statements, [](auto const &statement_) {
          return std::holds_alternative<ast::n_scheme::foreach_neighbor>(
              statement_);
        });

    outi() << "{ // foreach " << loop.group << " particle " << loop.index << nl;
    increase_indent();

    outi() << "PRTCL_RT_LOG_TRACE_SCOPED(\"foreach_particle\", \"p="
           << loop.group << "\");" << nl;
    out() << nl;

    if (has_foreach_neighbor_offspring) {
      outi() << "// select and resize the neighbor storage for the current "
                "thread"
             << nl;
      outi() << "auto &neighbors = t.neighbors;" << nl;
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
      // {{{ implementation
      if (not reductions.global.empty()) {
        outi() << "// global reductions" << nl;

        for (auto &[alias, op] : reductions.global.all()) {
          auto type = global.type_for_alias(alias);
          outi() << "auto &grd_" << alias << " = t.grd_" << alias << ";" << nl;
          outi() << "grd_" << alias << " = ";
          (*this)(rd_initializer(op, type));
          out() << ";" << nl;
        }

        out() << nl;
      }

      for (auto &[groups_name, set] : reductions.uniform) {
        outi() << "// uniform reductions (" << groups_name << ")" << nl;

        for (auto &[alias, op] : set.all()) {
          auto the_groups = groups.groups_for_name(groups_name);
          auto type = the_groups.uniform_fields.type_for_alias(alias);
          outi() << "auto &urd_" << groups_name << "_" << alias << " = t.urd_"
                 << groups_name << "_" << alias << ";" << nl;
          outi() << "urd_" << groups_name << "_" << alias << " = ";
          (*this)(rd_initializer(op, type));
          out() << ";" << nl;
        }

        out() << nl;
      }
      // }}}
    }

    outi() << "for (auto &p : _data.groups." << loop.group << ") {" << nl;
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
        for (auto const &statement : loop.statements) {
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
        // {{{ implementation
        increase_indent();

        if (not reductions.global.empty()) {
          outi() << "// global reductions" << nl;
          for (auto &[alias, op] : reductions.global.all()) {
            auto var = std::string{"grd_"} + alias;

            outi() << "g." << alias << "[0] ";

            switch (op) {
            case ast::op_max_assign:
              out() << " = o::max( g." << alias << "[0], ";
              break;
            case ast::op_min_assign:
              out() << " = o::min( g." << alias << "[0], ";
              break;
            default:
              (*this)(op);
            }

            out() << " " << var;

            switch (op) {
            case ast::op_max_assign:
            case ast::op_min_assign:
              out() << " );" << nl;
              break;
            default:
              out() << ';' << nl;
            }
          }

          if (not reductions.uniform.empty())
            out() << nl;
        }

        for (auto &[groups_name, set] : reductions.uniform) {
          outi() << "// uniform reductions (" << groups_name << ")" << nl;

          for (auto &[alias, op] : set.all()) {
            auto var = std::string{"urd_"} + groups_name + "_" + alias;

            outi() << "p." << alias << "[0] ";

            switch (op) {
            case ast::op_max_assign:
              out() << " = o::max( p." << alias << "[0], ";
              break;
            case ast::op_min_assign:
              out() << " = o::min( p." << alias << "[0], ";
              break;
            default:
              (*this)(op);
            }

            out() << " " << var;

            switch (op) {
            case ast::op_max_assign:
            case ast::op_min_assign:
              out() << " );" << nl;
              break;
            default:
              out() << ';' << nl;
            }
          }

          out() << nl;
        }

        decrease_indent();
        // }}}
      }
      outi() << "} // pragma omp critical" << nl;
    }

    decrease_indent();
    outi() << "} // foreach " << loop.group << " particle " << loop.index << nl;

    if (not cur_particle)
      throw "internal error: particle loop was already unset";
    else
      cur_particle.reset();
    // }}}
  }

  void operator()(ast::n_scheme::n_solve::setup const &node) {
    outi() << "// setup " << node.name << " into " << node.into << nl;
  };

  void operator()(ast::n_scheme::n_solve::product const &node) {
    outi() << "// product " << node.name << " with " << node.with << " into "
           << node.into << nl;
  };

  void operator()(ast::n_scheme::n_solve::apply const &node) {
    outi() << "// apply " << node.with << nl;
  };

  void operator()(ast::n_scheme::solve const &node) {
    outi() << "/* { solve not supported yet */" << nl;
    outi() << "// solver: " << node.solver << nl;
    outi() << "// type:   " << ndtype_template_args(node.type) << nl;
    outi() << "// groups: " << node.groups << nl;
    outi() << "// index:  " << node.index << nl;
    (*this)(node.right_hand_side);
    (*this)(node.guess);
    (*this)(node.preconditioner);
    (*this)(node.system);
    (*this)(node.apply);
    outi() << "/* } solve not supported yet */" << nl;
  }

  void operator()(ast::n_scheme::procedure const &proc) {
    out() << nl;
    cxx_access_modifier("public");
    outi() << "template <typename NHood_>" << nl;
    outi() << "void " << proc.name << "(NHood_ const &nhood_) {" << nl;
    { // {{{ implementation
      increase_indent();

      if (not global.empty()) {
        outi() << "// alias for the global data" << nl;
        outi() << "auto &g = _data.global;" << nl;
        out() << nl;
      }

      outi() << "// alias for the math_policy member types" << nl;
      outi() << "using o = typename math_policy::operations;" << nl;
      out() << nl;
      outi() << "// resize the per thread storage" << nl;
      outi()
          << "_per_thread.resize(static_cast<size_t>(omp_get_max_threads()));"
          << nl;

      bool in_parallel_region = false;
      auto begin_parallel_region = [this, &in_parallel_region]() {
        if (not in_parallel_region) {
          out() << nl;
          outi() << cxx_inline_pragma("omp parallel") << " {" << nl;
          increase_indent();

          outi() << "auto &t = "
                    "_per_thread[static_cast<size_t>(omp_get_thread_num())];"
                 << nl;
          out() << nl;
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
      for (auto const &statement : proc.statements) {
        // conditionally begin and close the parallel region
        if (std::holds_alternative<ast::n_scheme::foreach_particle>(statement))
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

  // Ignore scheme { groups ... { ... } }.
  void operator()(ast::groups const &) {}

  // Ignore scheme { global { ... } }.
  void operator()(ast::global const &) {}

  struct select_formatter {
    // {{{ implementation
    void operator()(ast::n_groups::select_atom const &node) {
      switch (node.kind) {
      case ast::n_groups::select_type:
        result += "(group.get_type() == \"" + node.name + "\")";
        break;
      case ast::n_groups::select_tag:
        result += "group.has_tag(\"" + node.name + "\")";
        break;
      default:
        throw ast_printer_error{};
      }
    }

    void operator()(ast::n_groups::select_unary_logic const &node) {
      switch (node.op) {
      case ast::op_negation:
        result += "not ";
        break;
      default:
        throw ast_printer_error{};
      }
    }

    void operator()(ast::n_groups::select_multi_logic const &node) {
      if (not node.right_hand_sides.empty())
        result += "(";

      (*this)(node.operand);

      for (auto const &rhs : node.right_hand_sides) {
        switch (rhs.op) {
        case ast::op_conjunction:
          result += " and ";
          break;
        case ast::op_disjunction:
          result += " or ";
          break;
        default:
          throw ast_printer_error{};
        }

        (*this)(rhs.operand);
      }

      if (not node.right_hand_sides.empty())
        result += ")";
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
    std::string result;
    // }}}
  };

  void operator()(ast::scheme const &arg) {
    // {{{ implementation
    auto reductions = find_reductions(arg);

    // find the global field aliases, names and types
    global = find_global_fields(arg);
    groups = find_groups(arg);

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
      outi() << "using dtype = prtcl::core::dtype;" << nl;
      out() << nl;
      outi() << "template <dtype DType_> using dtype_t =" << nl;
      {
        increase_indent();
        outi() << "typename type_policy::template dtype_t<DType_>;" << nl;
        decrease_indent();
      }
      out() << nl;
      outi() << "template <dtype DType_, size_t ...Ns_> using ndtype_t =" << nl;
      {
        increase_indent();
        outi() << "typename math_policy::template ndtype_t<DType_, Ns_...>;"
               << nl;
        decrease_indent();
      }
      out() << nl;
      outi() << "template <dtype DType_, size_t ...Ns_> using "
                "ndtype_data_ref_t ="
             << nl;
      {
        increase_indent();
        outi() << "typename data_policy::template ndtype_data_ref_t<DType_, "
                  "Ns_...>;"
               << nl;
        decrease_indent();
      }
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

      if (not global.empty()) {
        cxx_access_modifier("private");
        outi() << "struct global_data {" << nl;
        { // {{{
          increase_indent();

          using namespace core::range_adaptors;

          // declare the data reference to the field
          for (auto const &[alias, name, type] : global.all()) {
            outi() << "ndtype_data_ref_t<" << ndtype_template_args(type) << "> "
                   << alias << ";" << nl;
          }

          out() << nl;

          outi() << "static void _require(model_type &model) {" << nl;
          { // {{{
            increase_indent();

            for (auto const &[alias, name, type] : global.all()) {
              outi() << "model.template add_global<"
                     << ndtype_template_args(type) << ">(\"" << name << "\");"
                     << nl;
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          out() << nl;

          outi() << "void _load(model_type const &model) {" << nl;
          { // {{{
            increase_indent();

            for (auto const &[alias, name, type] : global.all()) {
              outi() << alias << " = "
                     << "model.template get_global<"
                     << ndtype_template_args(type) << ">(\"" << name << "\");"
                     << nl;
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          decrease_indent();
        } // }}}
        outi() << "};" << nl;
        out() << nl;
      }

      for (auto [name, groups] : groups.all()) {
        cxx_access_modifier("private");
        outi() << "struct groups_" << name << "_data {" << nl;
        { // {{{
          increase_indent();

          outi() << "// particle count of the selected group" << nl;
          outi() << "size_t _count;" << nl;
          outi() << "// index of the selected group" << nl;
          outi() << "size_t _index;" << nl;
          out() << nl;

          // {{{ members
          if (not groups.uniform_fields.empty()) {
            outi() << "// uniform fields" << nl;
            for (auto const &[alias, name, type] :
                 groups.uniform_fields.all()) {
              outi() << "ndtype_data_ref_t<" << ndtype_template_args(type)
                     << "> " << alias << ";" << nl;
            }
            out() << nl;
          }

          if (not groups.varying_fields.empty()) {
            outi() << "// varying fields" << nl;
            for (auto const &[alias, name, type] :
                 groups.varying_fields.all()) {
              outi() << "ndtype_data_ref_t<" << ndtype_template_args(type)
                     << "> " << alias << ";" << nl;
            }
            out() << nl;
          }
          // }}}

          outi() << "static bool _selects(group_type &group) {" << nl;
          { // {{{
            increase_indent();

            select_formatter formatter;
            formatter(groups.select);
            outi() << "return " << formatter.result << ";" << nl;

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          out() << nl;
          outi() << "static void _require(group_type &group) {" << nl;
          { // {{{
            increase_indent();

            auto &uf = groups.uniform_fields;
            auto &vf = groups.varying_fields;

            if (not uf.empty()) {
              outi() << "// uniform fields" << nl;
              for (auto const &[alias, name, type] : uf.all()) {
                outi() << "group.template add_uniform<"
                       << ndtype_template_args(type) << ">(\"" << name << "\");"
                       << nl;
              }
            }

            if (not uf.empty() and not vf.empty())
              out() << nl;

            if (not vf.empty()) {
              outi() << "// varying fields" << nl;
              for (auto const &[alias, name, type] : vf.all()) {
                outi() << "group.template add_varying<"
                       << ndtype_template_args(type) << ">(\"" << name << "\");"
                       << nl;
              }
            }

            decrease_indent();
          } // }}}
          outi() << "}" << nl;

          out() << nl;
          outi() << "void _load(group_type const &group) {" << nl;
          { // {{{
            increase_indent();

            outi() << "_count = group.size();" << nl;

            auto &uf = groups.uniform_fields;
            auto &vf = groups.varying_fields;

            if (not uf.empty()) {
              out() << nl;
              outi() << "// uniform fields" << nl;
              for (auto const &[alias, name, type] : uf.all()) {
                outi() << alias << " = "
                       << "group.template get_uniform<"
                       << ndtype_template_args(type) << ">(\"" << name << "\");"
                       << nl;
              }
            }

            if (not vf.empty()) {
              out() << nl;
              outi() << "// varying fields" << nl;
              for (auto const &[alias, name, type] : vf.all()) {
                outi() << alias << " = "
                       << "group.template get_varying<"
                       << ndtype_template_args(type) << ">(\"" << name << "\");"
                       << nl;
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

      cxx_access_modifier("private");
      outi() << "struct {" << nl;
      { // {{{
        increase_indent();

        if (not global.empty()) {
          outi() << "global_data global;" << nl;
          out() << nl;
        }

        if (not groups.empty()) {
          outi() << "struct {" << nl;
          {
            increase_indent();

            for (auto const &[name, groups] : groups.all()) {
              outi() << "std::vector<groups_" << name << "_data> " << name
                     << ";" << nl;
            }

            decrease_indent();
          }
          outi() << "} groups;" << nl;
        }

        decrease_indent();
      } // }}}
      outi() << "} _data;" << nl;

      out() << nl;

      outi() << "struct per_thread_data {" << nl;
      { // {{{
        increase_indent();

        outi() << "std::vector<std::vector<size_t>> neighbors;" << nl;

        if (not reductions.empty()) {
          if (not reductions.global.empty()) {
            out() << nl;
            outi() << "// global reductions" << nl;

            for (auto &alias : reductions.global.aliases()) {
              outi() << "ndtype_t<"
                     << ndtype_template_args(global.type_for_alias(alias))
                     << "> grd_" << alias << ";" << nl;
            }
          }

          for (auto &[groups_name, set] : reductions.uniform) {
            out() << nl;
            outi() << "// uniform reductions (" << groups_name << ")" << nl;

            auto &g = groups.groups_for_name(groups_name);

            for (auto &alias : set.aliases()) {
              outi() << "ndtype_t<"
                     << ndtype_template_args(
                            g.uniform_fields.type_for_alias(alias))
                     << "> urd_" << groups_name << "_" << alias << ";" << nl;
            }
          }
        }

        decrease_indent();
      } // }}}
      outi() << "};" << nl;

      out() << nl;

      outi() << "std::vector<per_thread_data> _per_thread;" << nl;
      outi() << "size_t _group_count;" << nl;

      out() << nl;

      cxx_access_modifier("public");
      outi() << "static void require(model_type &model) {" << nl;
      { // {{{
        increase_indent();

        outi() << "global_data::_require(model);" << nl;
        out() << nl;
        outi() << "for (auto &group : model.groups()) {" << nl;
        {
          increase_indent();

          for (auto &[name, groups] : groups.all()) {
            outi() << "if (groups_" << name << "_data::_selects(group))" << nl;
            {
              increase_indent();
              outi() << "groups_" << name << "_data::_require(group);" << nl;
              decrease_indent();
            }
          }

          decrease_indent();
        }
        outi() << "}" << nl;

        decrease_indent();
      } // }}}
      outi() << "}" << nl;

      outi() << nl;

      cxx_access_modifier("public");
      outi() << "void load(model_type &model) {" << nl;
      { // {{{
        increase_indent();

        outi() << "_group_count = model.groups().size();" << nl;
        out() << nl;

        if (not global.empty()) {
          outi() << "_data.global._load(model);" << nl;
          out() << nl;
        }

        if (not groups.empty()) {
          for (auto &[name, groups] : groups.all()) {
            outi() << "_data.groups." << name << ".clear();" << nl;
          }
          out() << nl;
        }

        outi() << "auto groups = model.groups();" << nl;
        outi() << "for (size_t i = 0; i < groups.size(); ++i) {" << nl;
        {
          increase_indent();

          outi() << "auto &group = groups[static_cast<typename "
                    "decltype(groups)::difference_type>(i)];"
                 << nl;

          for (auto &[name, groups] : groups.all()) {
            out() << nl;
            outi() << "if (groups_" << name << "_data::_selects(group)) {"
                   << nl;
            {
              increase_indent();

              outi() << "auto &data = _data.groups." << name
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

      // iterate over all child nodes
      for (auto const &statement : arg.statements)
        (*this)(statement);

      decrease_indent();
    }
    outi() << "};" << nl;

    out() << nl;

    { // close all nested namespaces
      using namespace core::range_adaptors;
      outi() << join(_namespaces | reversed, " ", "", [](auto ns_) {
        return "} /* namespace " + ns_ + "*/";
      }) << nl;
    }

    out() << _hpp_footer;

    // }}} implementation
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
  field_map global;
  groups_map groups;

  struct loop_type {
    std::string selector_name;
    std::string index_name;
  };
  std::optional<loop_type> cur_particle, cur_neighbor;
};

} // namespace prtcl::gt::printer
