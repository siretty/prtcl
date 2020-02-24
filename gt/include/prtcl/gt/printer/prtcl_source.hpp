#pragma once

#include "../bits/printer_crtp.hpp"

#include "../ast.hpp"

namespace prtcl::gt::printer {

struct prtcl_source : public printer_crtp<printer::prtcl_source> {
public:
  using base_type::base_type;
  using base_type::operator();

public:
  //! Print a constant.
  void operator()(ast::math::constant const &arg_) {
    out() << arg_.constant_name << '<' << arg_.constant_type << '>';
  }

  //! Print a literal. TODO: debug format
  void operator()(ast::math::literal const &arg_) {
    out() << arg_.type << '{' << arg_.value << '}';
  }

  //! Print access to a field (eg. x[f]).
  void operator()(ast::math::field_access const &arg_) {
    out() << arg_.field_name << '[' << arg_.index_name << ']';
  }

  //! Print a function call.
  void operator()(ast::math::function_call const &arg_) {
    out() << arg_.function_name << '(' << sep(arg_.arguments, ", ") << ')';
  }

  //! Print the arithmetic n-ary operation.
  void operator()(ast::math::arithmetic_nary const &arg_) {
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
  }

  void operator()(ast::stmt::reduce const &arg_) {
    out() << indent() << "reduce " << arg_.field_name << '[' << arg_.index_name
          << ']' << ' ' << arg_.op << ' ';
    (*this)(arg_.expression);
    out() << ';' << nl;
  }

  void operator()(ast::stmt::compute const &arg_) {
    out() << indent() << "compute " << arg_.field_name << '[' << arg_.index_name
          << ']' << ' ' << arg_.op << ' ';
    (*this)(arg_.expression);
    out() << ';' << nl;
  }

  void operator()(ast::stmt::foreach_neighbor const &arg_) {
    out() << indent() << "foreach " << arg_.selector_name << " neighbor "
          << arg_.neighbor_index_name << " {" << nl;
    increase_indent();
    for (auto const &statement : arg_.statements)
      (*this)(statement);
    decrease_indent();
    out() << indent() << '}' << nl;
  }

  void operator()(ast::stmt::foreach_particle const &arg_) {
    out() << indent() << "foreach " << arg_.selector_name << " particle "
          << arg_.particle_index_name << " {" << nl;
    increase_indent();
    for (auto const &statement : arg_.statements)
      (*this)(statement);
    decrease_indent();
    out() << indent() << '}' << nl;
  }

  void operator()(ast::stmt::procedure const &arg_) {
    out() << indent() << "procedure " << arg_.procedure_name << " {" << nl;
    increase_indent();
    for (auto const &statement : arg_.statements)
      (*this)(statement);
    decrease_indent();
    out() << indent() << '}' << nl;
  }

  void operator()(ast::init::field const &arg_) {
    out() << "field " << arg_.field_name << " : " << arg_.storage << ' '
          << arg_.field_type;
  }

  void operator()(ast::init::particle_selector const &arg_) {
    out() << "particle_selector types {" << sep(arg_.type_disjunction, " or ")
          << "} tags {" << sep(arg_.tag_conjunction, " and ") << '}';
  }

  void operator()(ast::stmt::let const &arg_) {
    out() << "let " << arg_.alias_name << " = ";
    (*this)(arg_.initializer);
    out() << ';' << nl;
  }

  void operator()(ast::prtcl_source_file const &arg_) {
    for (auto const &statement : arg_.statements)
      (*this)(statement);
  }
};

} // namespace prtcl::gt::printer
