#pragma once

#include "../common.hpp"

#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

// {{{ common type aliases
namespace prtcl::gt::ast {

using std::optional;
using std::string;
using std::tuple;
using std::vector;

using std::variant;
using nil = std::monostate;

} // namespace prtcl::gt::ast
// }}}

// ============================================================
// Synopsis and Forward Declaration
// ============================================================

namespace prtcl::gt::ast {

enum class basic_type;
enum class storage_qualifier;

struct nd_type;

namespace math {

enum class arithmetic_op;
enum class unary_op;

// n-ary arithmetic right-hand-side (... + x, ... - x, ... * x, ... / x)
struct arithmetic_nary_rhs;

// n-ary arithmetic operation
struct arithmetic_nary;

// unary operators (-x, +x)
struct unary;

struct literal;
struct constant;
struct field_access;

struct function_call;

using expression = variant<
    nil, literal, constant, field_access, function_call, arithmetic_nary,
    unary>;

} // namespace math

namespace init {

struct field;
struct particle_selector;

} // namespace init

namespace stmt {

enum class compute_op;
enum class reduce_op;

struct let;
struct local;
struct compute;
struct reduce;
struct foreach_neighbor;
struct foreach_particle;
struct procedure;

} // namespace stmt

using statement = variant<stmt::let, stmt::procedure>;

struct prtcl_source_file;

} // namespace prtcl::gt::ast

// ============================================================
// Definition and Implementation
// ============================================================

namespace prtcl::gt::ast {

enum class basic_type { real, integer, boolean };

// {{{ operator<<(ostream &, basic_type) -> ostream &
inline std::ostream &operator<<(std::ostream &stream_, basic_type value_) {
  switch (value_) {
  case basic_type::real:
    stream_ << "real";
    break;
  case basic_type::integer:
    stream_ << "integer";
    break;
  case basic_type::boolean:
    stream_ << "boolean";
    break;
  }
  return stream_;
}
// }}}

enum class storage_qualifier { global, uniform, varying };

// {{{ operator<<(ostream &, storage_qualifier) -> ostream &
inline std::ostream &
operator<<(std::ostream &stream_, storage_qualifier kind_) {
  switch (kind_) {
  case storage_qualifier::global:
    stream_ << "global";
    break;
  case storage_qualifier::uniform:
    stream_ << "uniform";
    break;
  case storage_qualifier::varying:
    stream_ << "varying";
    break;
  }
  return stream_;
}
// }}}

struct nd_type {
  basic_type type;
  vector<unsigned> shape;

  static nd_type const real, integer, boolean;
};

nd_type const nd_type::real = {basic_type::real, {}};
nd_type const nd_type::integer = {basic_type::integer, {}};
nd_type const nd_type::boolean = {basic_type::boolean, {}};

// {{{ operator<<(ostream &, nd_type) -> ostream &
inline std::ostream &operator<<(std::ostream &stream_, nd_type const &value_) {
  stream_ << value_.type;
  for (auto extent : value_.shape) {
    stream_ << '[';
    if (0 != extent)
      stream_ << extent;
    stream_ << ']';
  }
  return stream_;
}
// }}}

namespace math {

enum class arithmetic_op { add, sub, mul, div };

// {{{ operator<<(ostream &, arithmetic_op) -> ostream &
inline std::ostream &operator<<(std::ostream &stream_, arithmetic_op value_) {
  switch (value_) {
  case arithmetic_op::add:
    stream_ << "+";
    break;
  case arithmetic_op::sub:
    stream_ << "-";
    break;
  case arithmetic_op::mul:
    stream_ << "*";
    break;
  case arithmetic_op::div:
    stream_ << "/";
    break;
  }
  return stream_;
}
// }}}

struct arithmetic_nary_rhs {
  arithmetic_op op;
  value_ptr<expression> rhs;
};

struct arithmetic_nary {
  value_ptr<expression> first_operand;
  vector<arithmetic_nary_rhs> right_hand_sides;
};

enum class unary_op { neg, pos };

// {{{ operator<<(ostream &, unary_op) -> ostream &
inline std::ostream &operator<<(std::ostream &stream_, unary_op value_) {
  switch (value_) {
  case unary_op::neg:
    stream_ << "-";
    break;
  case unary_op::pos:
    stream_ << "+";
    break;
  }
  return stream_;
}
// }}}

struct unary {
  unary_op op;
  value_ptr<expression> operand;
};

struct literal {
  nd_type type;
  string value;
};

struct constant {
  string constant_name;
  nd_type constant_type;
};

struct field_access {
  string field_name;
  string index_name;
};

struct function_call {
  string function_name;
  vector<value_ptr<expression>> arguments;
};

} // namespace math

namespace init {

struct field {
  string field_name;
  storage_qualifier storage;
  nd_type field_type;
};

// {{{ operator<<(ostream &, field) -> ostream &
inline std::ostream &operator<<(std::ostream &stream_, field const &value_) {
  return stream_ << "field" << ' ' << value_.field_name << " : "
                 << value_.storage << ' ' << value_.field_name;
}
// }}}

struct particle_selector {
  vector<string> type_disjunction;
  vector<string> tag_conjunction;
};

// {{{ operator<<(ostream &, particle_selector) -> ostream &
inline std::ostream &
operator<<(std::ostream &stream_, particle_selector const &value_) {
  stream_ << "particle_selector" << ' ';
  stream_ << "types" << ' ' << '[';
  if (not value_.type_disjunction.empty()) {
    for (auto const &type : value_.type_disjunction)
      stream_ << ' ' << type;
    stream_ << ' ';
  }
  stream_ << ']' << ' ';
  stream_ << "tags" << ' ' << '[';
  if (not value_.type_disjunction.empty()) {
    for (auto const &tag : value_.tag_conjunction)
      stream_ << ' ' << tag;
    stream_ << ' ';
  }
  stream_ << ']';
  return stream_;
}
// }}}

} // namespace init

namespace bits {

using foreach_neighbor_statement = variant<stmt::local, stmt::compute, stmt::reduce>;

using foreach_particle_statement =
    variant<stmt::local, stmt::compute, stmt::reduce, stmt::foreach_neighbor>;

using procedure_statement = variant<stmt::local, stmt::compute, stmt::foreach_particle>;

} // namespace bits

namespace stmt {

struct let {
  string alias_name;
  variant<init::field, init::particle_selector> initializer;
};

struct local {
  string local_name;
  nd_type local_type;
  math::expression expression;
};

enum class compute_op {
  assign,
  add_assign,
  sub_assign,
  mul_assign,
  div_assign,
  max_assign,
  min_assign
};

// {{{ operator<<(ostream &, compute_op) -> ostream &
inline std::ostream &
operator<<(std::ostream &stream_, compute_op const &value_) {
  switch (value_) {
  case compute_op::assign:
    stream_ << '=';
    break;
  case compute_op::add_assign:
    stream_ << "+=";
    break;
  case compute_op::sub_assign:
    stream_ << "-=";
    break;
  case compute_op::mul_assign:
    stream_ << "*=";
    break;
  case compute_op::div_assign:
    stream_ << "/=";
    break;
  case compute_op::max_assign:
    stream_ << "max=";
    break;
  case compute_op::min_assign:
    stream_ << "min=";
    break;
  };
  return stream_;
}
// }}}

struct compute {
  string field_name;
  string index_name;
  compute_op op;
  math::expression expression;
};

enum class reduce_op {
  add_assign,
  sub_assign,
  mul_assign,
  div_assign,
  max_assign,
  min_assign
};

// {{{ operator<<(ostream &, reduce_op) -> ostream &
inline std::ostream &
operator<<(std::ostream &stream_, reduce_op const &value_) {
  switch (value_) {
  case reduce_op::add_assign:
    stream_ << "+=";
    break;
  case reduce_op::sub_assign:
    stream_ << "-=";
    break;
  case reduce_op::mul_assign:
    stream_ << "*=";
    break;
  case reduce_op::div_assign:
    stream_ << "/=";
    break;
  case reduce_op::max_assign:
    stream_ << "max=";
    break;
  case reduce_op::min_assign:
    stream_ << "min=";
    break;
  };
  return stream_;
}
// }}}

struct reduce {
  string field_name;
  string index_name;
  reduce_op op;
  math::expression expression;
};

struct foreach_neighbor {
  string selector_name;
  string neighbor_index_name;
  vector<bits::foreach_neighbor_statement> statements;
};

struct foreach_particle {
  string selector_name;
  string particle_index_name;
  vector<bits::foreach_particle_statement> statements;
};

struct procedure {
  string procedure_name;
  vector<bits::procedure_statement> statements;
};

} // namespace stmt

struct prtcl_source_file {
  string version;
  vector<statement> statements;
};

} // namespace prtcl::gt::ast
