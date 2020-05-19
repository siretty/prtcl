local module = {}


local lpeg = require "lpeg"
local P, V = lpeg.P, lpeg.V
local C, Ct, Cg = lpeg.C, lpeg.Ct, lpeg.Cg


local pgc = require "prtcl.grammar.common"
local WS0, WS1 = pgc.WS0, pgc.WS1


local NEG = P("-")
local ADD, SUB, MUL, DIV = P("+"), P("-"), P("*"), P("/")

local EQ, ADDEQ, SUBEQ, MULEQ, DIVEQ, MAXEQ, MINEQ =
  P("="), P("+="), P("-="), P("*="), P("/="), P("max="), P("min=")

local OPEQ = ADDEQ + SUBEQ + MULEQ + MAXEQ + MINEQ
module.OPEQ = OPEQ


local expression = V("expression")
local atom, slice = V("atom"), V("slice")
local add_term, mul_term, neg_term = V("add_term"), V("mul_term"), V("neg_term")


module.grammar = P{"start",
  start = expression,

  atom =
      pgc.block("field",
        pgc.Identifier("name")
        * WS0 * P(".") * WS0 *
        pgc.Identifier("from")
      )
    +
      pgc.block("call",
          pgc.Identifier("name")
        *
          Cg(
            (P("<") * pgc.ndtype * P(">"))^-1,
            "type"
          )
        *
          P("(") * WS0
          *
          Cg(
            Ct(
              (expression * (P(",") * expression)^0)^-1
            ),
            "arguments"
          )
          *
          WS0 * P(")")
      )
    +
      pgc.block("ident",
        pgc.Identifier("name")
      )
    +
      WS0 * P("(") * WS0 
      * expression *
      WS0 * P(")") * WS0,

  slice =
      pgc.block("slice",
        atom * WS0
        *
          Cg(Ct(
            P("[") * WS0
              * C(pgc.UINT)
            * WS0 * P("]")
          ), "indices")
      )
    +
      atom,

  neg_term =
      pgc.unary("neg",
        NEG * slice
      )
    +
      slice,

  mul_term = pgc.multary("mul",
    neg_term * (C(MUL + DIV) * neg_term)^0
  ),

  add_term = pgc.infix("infix", C(ADD + SUB), mul_term),

  expression = add_term
}

return module

