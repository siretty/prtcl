local module = {}


local lpeg = require "lpeg"
local P, S, R, V = lpeg.P, lpeg.S, lpeg.R, lpeg.V
local C, Ct, Cc, Cg = lpeg.C, lpeg.Ct, lpeg.Cc, lpeg.Cg


local SCOMMENT = lpeg.P("//") * (lpeg.P(1) - lpeg.P("\n"))^0
local MCOMMENT = lpeg.P("/*") * (lpeg.P(1) - lpeg.P("*/"))^0 * lpeg.P("*/")

local WHITESPACE = lpeg.S(" \n\t") + SCOMMENT + MCOMMENT
local WS0 = WHITESPACE^0
local WS1 = WHITESPACE^1

local BOOL = P("true") + P("false")
local UINT = R("09")^1
local SINT = P("-")^-1 * UINT
local REAL = P("-")^-1 * (UINT^-1 * P(".") * UINT + UINT * (P(".") * UINT)^-1)

local DTYPE = P("real") + P("integer") + P("boolean")

local IDENT = R("az", "AZ") * R("az", "AZ", "09", "__")^0

module.WS0, module.WS1 = WS0, WS1
module.BOOL, module.UINT, module.SINT, module.REAL = BOOL, UINT, SINT, REAL
module.DTYPE, module.IDENT = DTYPE, IDENT


function module.Identifier(name) 
  return Cg(C(IDENT), name)
end


local function expect(expected)
  -- this pattern matches the empty string and "captures" the first
  -- extra argument to match(...)
  local pattern = lpeg.Carg(1)
  -- use a match-time capture to update the state whenever the parser
  -- hits the result of this function
  return lpeg.Cmt(
    pattern,
    function(subject, position, state)
      -- update the state
      state.last_expected = expected
      state.last_position = position
      -- do not change the position of the parser and "capture" what
      -- has been set as expected in the state
      return true, expected
    end
  )
end

local function raw_block(_type, pattern)
  return lpeg.Ct(
    lpeg.Cg(expect(_type), "_type")
  *
    pattern
  )
end


function module.block(_type, pattern)
  return raw_block(_type, WS0 * pattern * WS0)
end

function module.statement(_type, pattern)
  return raw_block(_type, WS0 * pattern * WS0 * (";" * WS0)^1)
end


function module.unary(_type, pattern)
  return lpeg.Ct(
    lpeg.Cg(expect(_type), "_type")
  *
    lpeg.Cg(pattern, "operand")
  ) / function(obj)
    assert(type(obj.operand) == "table")
    assert(obj.operand._type ~= nil)
    return obj
  end
end

function module.multary(_type, pattern)
  return lpeg.Ct(
    lpeg.Cg(expect(_type), "_type")
  *
    Cg(Ct(pattern), "operands")
  ) / function(obj)
    assert(type(obj.operands) == "table")
    assert(obj.operands._type == nil)
    assert(#obj.operands > 0)
    if #obj.operands == 1 then
      return obj.operands[1]
    end
    return obj
  end
end

function module.infix(_type, operator, argument)
  return lpeg.Ct(
    lpeg.Cg(expect(_type), "_type")
  *
    Cg(Ct(argument * (operator * argument)^0), "operands")
  ) / function(obj)
    assert(type(obj.operands) == "table")
    assert(obj.operands._type == nil)
    assert(#obj.operands > 0)
    if #obj.operands == 1 then
      return obj.operands[1]
    end
    return obj
  end
end


-- Collect zero or more repetitions of \p pattern in a capture group
-- called \p key.
function module.collect(key, pattern)
  return lpeg.Cg(lpeg.Ct(pattern^0), key)
end


module.ndtype = module.block("ndtype",
  Cg(C(DTYPE), "type")
  *
  module.collect("extents",
    WS0 * P("[") * WS0
    *
    (
      C( R("09")^1 ) + Cc(0) 
    )
    *
    WS0 * P("]")
  )
)

return module
