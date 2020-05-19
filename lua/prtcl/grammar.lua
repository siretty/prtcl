local module = {}

local lpeg = require "lpeg"
local P, S, R, V = lpeg.P, lpeg.S, lpeg.R, lpeg.V
local C, Ct, Cc, Cg = lpeg.C, lpeg.Ct, lpeg.Cc, lpeg.Cg


local helper = require "prtcl.helper"
local collect = helper.collect


local pgc = require "prtcl.grammar.common"
local pg_select = require "prtcl.grammar.select"
local pg_math = require "prtcl.grammar.math"


local block, statement, collect = pgc.block, pgc.statement, pgc.collect

local Identifier = pgc.Identifier

local WS0, WS1 = pgc.WS0, pgc.WS1

local BOOL = P("true") + P("false")
local UINT = R("09")^1
local SINT = P("-")^-1 * UINT
local REAL = P("-")^-1 * (UINT^-1 * P(".") * UINT + UINT * (P(".") * UINT)^-1)

local DTYPE = P("real") + P("integer") + P("boolean")

local IDENT = R("az", "AZ") * R("az", "AZ", "09", "__")^0


function Identifier(name) 
  return Cg(C(IDENT), name)
end


module.grammar = P{
  "start",

  start = V("scheme")^1,

  ndtype = pgc.ndtype,

  scheme = block("scheme",
    P("scheme") * WS1
        * Identifier("name") * WS0
        * P("{") * WS0
    *
    collect("statements",
      V("groups") + V("global") + V("procedure")
    )
    *
    WS0 * P("}")
  ),

  groups = block("groups",
    P("groups") * WS1
        * Identifier("name") * WS0
        * P("{") * WS0
    *
      Cg(V("_select"), "select")
    *
      collect("fields",
        V("groups_field")
      )
    *
    WS0 * P("}")
  ),

  global = block("global",
    P("global") * WS0 * P("{") * WS0
    *
    collect("fields",
      V("global_field") 
    )
    *
    WS0 * P("}")
  ),

  _select = pg_select.grammar,

  groups_field = statement("field",
    Cg(P("uniform") + P("varying"), "kind")
    * WS1 *
    V("_field")
  ),

  global_field = statement("field",
    Cg(Cc("uniform"), "kind")
    *
    V("_field")
  ),

  _field = (
    P("field")
    * WS1 *
    Identifier("alias")
    * WS0 *
    P("=")
    * WS0 *
    Cg(V("ndtype"), "type")
    * WS0 *
    Identifier("name")
  ),

  procedure = block("procedure",
    P("procedure") * WS1
        * Identifier("name") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("compute") + V("foreach_particle") + V("solve")
    )
    * 
    WS0 * P("}")
  ),

  foreach_particle = block("foreach_particle",
    P("foreach") * WS1
        * Identifier("groups") * WS1
        * P("particle") * WS1
        * Identifier("index") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("reduce") + V("compute") + V("foreach_neighbor")
    )
    *
    WS0 * P("}")
  ),

  foreach_neighbor = block("foreach_neighbor",
    P("foreach") * WS1
        * Identifier("groups") * WS1
        * P("neighbor") * WS1
        * Identifier("index") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("reduce") + V("compute") + V("foreach_neighbor")
    )
    *
    WS0 * P("}")
  ),

  local_ = statement("local_",
    P("local") * WS1 * (P(1) - P(";"))^0
  ),

  compute = statement("compute",
    P("compute") * WS1 * (P(1) - P(";"))^0
  ),

  reduce = statement("reduce",
      P("reduce") * WS0
    *
      Cg(pg_math.grammar, "into")
    * WS0 *
      Cg(C(pg_math.OPEQ), "operator")
    * WS0 *
      Cg(pg_math.grammar, "math")
  ),

  solve = block("solve",
    P("solve") * WS1
        * Identifier("solver") * WS1
        * Cg(V("ndtype"), "type") * WS0
        * P("over") * WS1
        * Identifier("groups") * WS1
        * P("particle") * WS1
        * Identifier("index") * WS0
        * P("{")
    *
    collect("statements",
      V("setup") + V("product") + V("apply")
    )
    *
    WS0 * P("}")
  ),

  setup = block("setup",
    P("setup") * WS1
        * Identifier("name") * WS1
        * P("into") * WS1
        * Identifier("into") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("compute") + V("foreach_neighbor")
    )
    *
    P("}")
  ),

  product = block("product",
    P("product") * WS1
        * Identifier("name") * WS1
        * P("with") * WS1
        * Identifier("with") * WS1
        * P("into") * WS1
        * Identifier("into") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("compute") + V("foreach_neighbor")
    )
    *
    P("}")
  ),

  apply = block("apply",
    P("apply") * WS1
        * Identifier("with") * WS0
        * P("{")
    *
    collect("statements",
      V("local_") + V("compute") + V("foreach_neighbor")
    )
    *
    P("}")
  ),
}


function module.parse(subject)
  local state = {
    last_position = nil,
    last_expected = nil,
  }

  local result = {module.grammar:match(subject, 1, state)}

  if state.last_position ~= nil and state.last_position < #subject then
    print("subject was not fully matched")
    print("expected '" .. state.last_expected .. "' at position " .. state.last_position)
  end

  return result
end


local formatters = {
  scheme = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "scheme " .. data.name)
    for _, v in pairs(data.statements) do
      format(v, indent + 1, format)
    end
  end,

  groups = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "groups " .. data.name)
    print(space .. "  select: ...")
    print(space .. "  fields:")
    for _, v in pairs(data.fields) do
      format(v, indent + 2, format)
    end
  end,

  global = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "global")
    print(space .. "  fields:")
    for _, v in pairs(data.fields) do
      format(v, indent + 2, format)
    end
  end,

  field = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. data.kind .. " " .. format(data.type) .. " " .. data.name .. " " .. data.alias)
  end,

  ndtype = function(data, indent, format)
    local result = data.type
    for _, extent in pairs(data.extents) do
      result = result .. "[" .. extent .. "]"
    end
    return result
  end,

  procedure = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "procedure " .. data.name)
    for _, v in pairs(data.statements) do
      format(v, indent + 1, format)
    end
  end,

  foreach_particle = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "foreach_particle")
  end,

  foreach_neighbor = function(data, indent, format)
    local space = string.rep("  ", indent)

    print(space .. "foreach_neighbor")
  end,

  solve = function(data, indent, format)
  end,
}


function module.pretty_print(tree, indent)
  helper.pprint(tree)
  --[[
  if indent == nil then indent = 0 end
  local space = string.rep("  ", indent)

  if tree == nil then
    print(space .. "nil")
    return
  end

  if type(tree) ~= "table" then
    print(space .. tree)
    return
  end
  
  if tree._type ~= nil then
    return formatters[tree._type](tree._data, indent, module.pretty_print)
  else
    for k, v in pairs(tree) do
      print(space .. k .. ":")
      module.pretty_print(v, indent + 1)
    end
  end
  --]]
end


return module
