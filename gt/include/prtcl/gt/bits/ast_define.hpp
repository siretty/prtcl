#pragma once

#include "../common.hpp"

#include <prtcl/core/ndtype.hpp>

#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>

// {{{ common type aliases
namespace prtcl::gt::ast {

using std::optional;
using std::string;
using std::tuple;
using std::vector;

using std::variant;
using nil = std::monostate;

using boost::spirit::x3::position_tagged;

} // namespace prtcl::gt::ast
// }}}

// ============================================================
// Synopsis and Forward Declaration
// ============================================================

namespace prtcl::gt::ast {

using core::dtype;
using core::ndtype;

static core::ndtype const ndtype_real = ndtype{dtype::real, {}};
static core::ndtype const ndtype_integer = ndtype{dtype::integer, {}};
static core::ndtype const ndtype_boolean = ndtype{dtype::boolean, {}};

enum multi_logic_op { op_conjunction, op_disjunction };
enum unary_logic_op { op_negation };

enum multi_arithmetic_op { op_add, op_sub, op_mul, op_div };
enum unary_arithmetic_op { op_neg };

enum assign_op {
  op_assign,
  op_add_assign,
  op_sub_assign,
  op_mul_assign,
  op_div_assign,
  op_max_assign,
  op_min_assign
};

namespace n_math {
// {{{

struct literal;
struct operation;
struct field_access;

// unary arithmetic expression
struct unary_arithmetic;

// n-ary arithmetic expression
struct multi_arithmetic_rhs;
struct multi_arithmetic;

using expression = variant<
    nil, literal, operation, field_access, unary_arithmetic, multi_arithmetic>;

// }}}
} // namespace n_math

namespace n_global {
// {{{

struct field;

// }}}
} // namespace n_global

struct global;

namespace n_group {
// {{{

enum select_atom_kind { select_type, select_tag };

// atomic expressions
struct select_atom;

// unary logical expression
struct unary_logic;

// n-ary logical expression
struct multi_logic_rhs;
struct multi_logic;

using expression = variant<nil, select_atom, unary_logic, multi_logic>;

struct uniform_field;
struct varying_field;

using field = variant<nil, uniform_field, varying_field>;

// }}}
} // namespace n_group

struct group;

namespace n_scheme {
// {{{

struct local;
struct compute;
struct reduce;

struct foreach_neighbor;
struct foreach_particle;
struct procedure;

// }}}
} // namespace n_scheme

struct scheme;

} // namespace prtcl::gt::ast

// ============================================================
// Definition and Implementation
// ============================================================

namespace prtcl::gt::ast {

// {{{ math details

namespace n_math {

// {{{ literal, operation, field_access

struct literal {
  ndtype type;
  string value; // TODO
};

struct operation {
  string name;
  optional<ndtype> type;
  vector<value_ptr<expression>> arguments;
};

struct field_access {
  string field;
  optional<string> index;
};

// }}}

// {{{ unary_arithmetic, multi_arithmetic_rhs, multi_arithmetic

struct unary_arithmetic {
  unary_arithmetic_op op;
  value_ptr<expression> operand;
};

struct multi_arithmetic_rhs {
  multi_arithmetic_op op;
  value_ptr<expression> operand;
};

struct multi_arithmetic {
  value_ptr<expression> operand;
  vector<multi_arithmetic_rhs> right_hand_sides;
};

// }}}

} // namespace n_math

// }}}

// {{{ global details

namespace n_global {

struct field {
  string alias;
  ndtype type;
  string name;
};

} // namespace n_global

// }}}

struct global {
  vector<n_global::field> fields;
  int __dummy;
};

// {{{ group details

namespace n_group {

// {{{ select_atom

struct select_atom {
  select_atom_kind kind;
  string name;
};

// }}}

// {{{ unary_logic, multi_logic_rhs, multi_logic

struct unary_logic {
  unary_logic_op op;
  value_ptr<expression> operand;
};

struct multi_logic_rhs {
  multi_logic_op op;
  value_ptr<expression> operand;
};

struct multi_logic {
  value_ptr<expression> operand;
  vector<multi_logic_rhs> right_hand_sides;
};

// }}}

// {{{ uniform_field, varying_field

struct uniform_field {
  string alias;
  ndtype type;
  string name;
};

struct varying_field {
  string alias;
  ndtype type;
  string name;
};

// }}}

} // namespace n_group

// }}}

struct group {
  string name;
  n_group::expression select;
  vector<n_group::field> fields;
};

// {{{ scheme details

namespace n_scheme {

// {{{ local, compute, reduce

struct local {
  string name;
  ndtype type;
  n_math::expression math;
};

struct compute {
  n_math::field_access left_hand_side;
  assign_op op;
  n_math::expression math;
};

struct reduce {
  n_math::field_access left_hand_side;
  assign_op op;
  n_math::expression math;
};

// }}}

// {{{ foreach_neighbor, foreach_particle, procedure

struct foreach_neighbor {
  using statement = variant<local, compute, reduce>;

  string group;
  string index;
  vector<statement> statements;
};

struct foreach_particle {
  using statement = variant<local, compute, reduce, foreach_neighbor>;

  string group;
  string index;
  vector<statement> statements;
};

struct procedure {
  using statement = variant<local, compute, foreach_particle>;

  string name;
  vector<statement> statements;
};

// }}}

} // namespace n_scheme

// }}}

struct scheme {
  using statement = variant<nil, global, group, n_scheme::procedure>;

  string name;
  vector<statement> statements;
};

struct prtcl_file {
  using statement = variant<nil, scheme>;

  string version;
  vector<statement> statements;
};

} // namespace prtcl::gt::ast
