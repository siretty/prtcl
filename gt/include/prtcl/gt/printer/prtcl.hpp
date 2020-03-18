#pragma once

#include "../bits/printer_crtp.hpp"

#include "../ast.hpp"
#include "prtcl/gt/bits/ast_define.hpp"
#include <variant>

namespace prtcl::gt::printer {

struct prtcl : public printer_crtp<printer::prtcl> {
public:
  using base_type::base_type;
  using base_type::operator();

public:
  void operator()(ast::dtype const &arg) {
    switch (arg) {
    case ast::dtype::real:
      out() << "real";
      break;
    case ast::dtype::integer:
      out() << "integer";
      break;
    case ast::dtype::boolean:
      out() << "boolean";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::ndtype const &arg) {
    (*this)(arg.type);
    for (auto const &extent : arg.shape) {
      out() << '[';
      if (extent > 0)
        out() << extent;
      out() << ']';
    }
  }

  void operator()(ast::unary_logic_op const &arg) {
    switch (arg) {
    case ast::op_negation:
      out() << "not";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::unary_arithmetic_op const &arg) {
    switch (arg) {
    case ast::op_neg:
      out() << "-";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::multi_logic_op const &arg) {
    switch (arg) {
    case ast::op_conjunction:
      out() << "and";
      break;
    case ast::op_disjunction:
      out() << "or";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::multi_arithmetic_op const &arg) {
    switch (arg) {
    case ast::op_add:
      out() << "+";
      break;
    case ast::op_sub:
      out() << "-";
      break;
    case ast::op_mul:
      out() << "*";
      break;
    case ast::op_div:
      out() << "/";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::assign_op const &arg) {
    switch (arg) {
    case ast::op_assign:
      out() << "=";
      break;
    case ast::op_add_assign:
      out() << "+=";
      break;
    case ast::op_sub_assign:
      out() << "-=";
      break;
    case ast::op_mul_assign:
      out() << "*=";
      break;
    case ast::op_div_assign:
      out() << "/=";
      break;
    case ast::op_max_assign:
      out() << "max=";
      break;
    case ast::op_min_assign:
      out() << "min=";
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::group const &arg) {
    outi() << "group " << arg.name << ' ' << '{' << nl;
    increase_indent();

    outi() << "select ";
    (*this)(arg.select);
    out() << ';' << nl;

    out() << nl;

    for (auto const &field : arg.fields)
      (*this)(field);

    decrease_indent();
    outi() << '}' << nl;
  }

  void operator()(ast::n_group::multi_logic const &arg) {
    if (arg.right_hand_sides.size() > 1)
      out() << "(";

    (*this)(arg.operand);

    for (auto const &rhs : arg.right_hand_sides) {
      out() << ' ';
      (*this)(rhs.op);
      out() << ' ';
      (*this)(rhs.operand);
    }

    if (arg.right_hand_sides.size() > 1)
      out() << ")";
  }

  void operator()(ast::n_group::select_atom const &arg) {
    switch (arg.kind) {
    case ast::n_group::select_tag:
      out() << "tag " << arg.name;
      break;
    case ast::n_group::select_type:
      out() << "type " << arg.name;
      break;
    default:
      throw ast_printer_error{};
    }
  }

  void operator()(ast::n_group::uniform_field const &arg) {
    outi() << "uniform field " << arg.alias << " = ";
    (*this)(arg.type);
    out() << " " << arg.name << ';' << nl;
  }

  void operator()(ast::n_group::varying_field const &arg) {
    outi() << "varying field " << arg.alias << " = ";
    (*this)(arg.type);
    out() << " " << arg.name << ';' << nl;
  }

  void operator()(ast::global const &arg) {
    outi() << "global {" << nl;
    increase_indent();

    for (auto const &field : arg.fields)
      (*this)(field);

    decrease_indent();
    outi() << '}' << nl;
  }

  void operator()(ast::n_global::field const &arg) {
    outi() << "field " << arg.alias << " = ";
    (*this)(arg.type);
    out() << ' ' << arg.name << ';' << nl;
  }

  //! Print a literal. TODO: debug format
  void operator()(ast::n_math::literal const &arg) {
    (*this)(arg.type);
    out() << '{' << arg.value << '}';
  }

  //! Print a function call.
  void operator()(ast::n_math::operation const &arg) {
    out() << arg.name;
    if (arg.type.has_value()) {
      out() << '<';
      (*this)(arg.type.value());
      out() << '>';
    }
    out() << '(' << sep(arg.arguments, ", ") << ')';
  }

  //! Print access to a field (eg. x[f]).
  void operator()(ast::n_math::field_access const &arg) {
    out() << arg.field;
    if (arg.index.has_value())
      out() << '[' << arg.index.value() << ']';
  }

  //! Print the unary arithmetic expression.
  void operator()(ast::n_math::unary_arithmetic const &arg) {
    (*this)(arg.op);
    //(*this)(arg.operand);
  }

  //! Print the n-ary arithmetic expression.
  void operator()(ast::n_math::multi_arithmetic const &arg) {
    if (not arg.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << '(';

    // print the first operand
    (*this)(arg.operand);

    // print the right-hand sides
    for (auto const &rhs : arg.right_hand_sides) {
      // print the operator
      out() << ' ';
      (*this)(rhs.op);
      out() << ' ';

      // print the operand
      (*this)(rhs.operand);
    }

    if (not arg.right_hand_sides.empty())
      // enclose in braces for multiple operands
      out() << ')';
  }

  void operator()(ast::n_scheme::compute const &arg) {
    outi() << "compute ";
    (*this)(arg.left_hand_side);
    out() << ' ';
    (*this)(arg.op);
    out() << ' ';
    (*this)(arg.math);
    out() << ';' << nl;
  }

  void operator()(ast::n_scheme::reduce const &arg) {
    out() << indent() << "reduce ";
    (*this)(arg.left_hand_side);
    out() << ' ';
    (*this)(arg.op);
    out() << ' ';
    (*this)(arg.math);
    out() << ';' << nl;
  }

  void operator()(ast::n_scheme::foreach_neighbor const &arg) {
    out() << indent() << "foreach " << arg.group << " neighbor " << arg.index
          << " {" << nl;
    increase_indent();
    for (auto const &statement : arg.statements) {
      (*this)(statement);
    }
    decrease_indent();
    out() << indent() << '}' << nl;
  }

  void operator()(ast::n_scheme::foreach_particle const &arg) {
    out() << indent() << "foreach " << arg.group << " particle " << arg.index
          << " {" << nl;
    increase_indent();

    bool needs_blank_line = false;
    size_t index = 0;
    for (auto const &statement : arg.statements) {
      size_t statement_index = index++;
      if (statement_index > 0) {
        bool statement_is_loop =
            std::holds_alternative<ast::n_scheme::foreach_neighbor>(statement);
        if (needs_blank_line) {
          out() << nl;
          needs_blank_line = statement_is_loop;
        } else if (statement_is_loop) {
          out() << nl;
          needs_blank_line = true;
        }
      }
      (*this)(statement);
    }

    decrease_indent();
    out() << indent() << '}' << nl;
  }

  void operator()(ast::n_scheme::procedure const &arg) {
    outi() << "procedure " << arg.name << ' ' << '{' << nl;
    increase_indent();

    bool needs_blank_line = false;
    size_t index = 0;
    for (auto const &statement : arg.statements) {
      size_t statement_index = index++;
      if (statement_index > 0) {
        bool statement_is_loop =
            std::holds_alternative<ast::n_scheme::foreach_particle>(statement);
        if (needs_blank_line) {
          out() << nl;
          needs_blank_line = statement_is_loop;
        } else if (statement_is_loop) {
          out() << nl;
          needs_blank_line = true;
        }
      }
      (*this)(statement);
    }

    decrease_indent();
    outi() << '}' << nl;
  }

  void operator()(ast::scheme const &arg) {
    out() << "scheme " << arg.name << ' ' << '{' << nl;
    increase_indent();
    size_t index = 0;
    for (auto const &statement : arg.statements) {
      if (index++ > 0)
        out() << nl;
      (*this)(statement);
    }
    decrease_indent();
    out() << '}' << nl;
  }

  void operator()(ast::prtcl_file const &arg) {
    out() << "// .prtcl file version: " << arg.version << nl;
    for (auto const &statement : arg.statements) {
      out() << nl;
      (*this)(statement);
    }
  }
};

} // namespace prtcl::gt::printer
