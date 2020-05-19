local module = {}


local lpeg = require "lpeg"
local P, V = lpeg.P, lpeg.V
local C, Cg = lpeg.C, lpeg.Cg


local pgc = require "prtcl.grammar.common"
local WS0, WS1 = pgc.WS0, pgc.WS1


local DIS, CON, NOT = P("or"), P("and"), P("not")

local atom, expression = V("atom"), V("expression")
local not_term, con_term, dis_term = V("not_term"), V("con_term"), V("dis_term")


module.grammar = P{"start",
  start = P("select") * WS1 * expression * WS0 * P(";"),

  atom =
      pgc.block("select_atom",
        Cg(P("type") + P("tag"), "kind")
        * WS1 *
        pgc.Identifier("name")
      )
    +
      WS0 * P("(") * WS0 
      * expression *
      WS0 * P(")") * WS0,

  not_term =
      pgc.unary("select_not",
        NOT * atom
      )
    +
      atom,

  con_term = pgc.multary("select_con", not_term * (CON * not_term)^0),
  dis_term = pgc.multary("select_dis", con_term * (DIS * con_term)^0),
  expression = dis_term
}

return module
