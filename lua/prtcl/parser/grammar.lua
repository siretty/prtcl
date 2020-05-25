local grammar = {}

local lpeg = require "lpeg"
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V
local C, Ct = lpeg.C, lpeg.Ct

local node = require "prtcl.parser.node"
local id = require "prtcl.parser.identifier"

local WS_SP = lpeg.S" \t"
local WS_NL = lpeg.S"\r\n"

local WS = (
    -- one or more whitespace characters
    (WS_SP + WS_NL)
  +
    -- single line comment
    P("//") * (lpeg.P(1) - lpeg.P("\n"))^0
  +
    -- multi line comment
    P("/*") * (lpeg.P(1) - lpeg.P("*/"))^0 * P("*/")
)^0

local EOS = P(-1)

local DIGIT10, DIGIT16 = R("09"), R("09", "af", "AF")
local EXPONENT = S("eE") * S("+-")^-1 * DIGIT10^1

local number = C(
      DIGIT10^1 * P(".") * DIGIT10^0 * EXPONENT^-1
    +
      DIGIT10^0 * P(".") * DIGIT10^1 * EXPONENT^-1
    +
      DIGIT10^1 * EXPONENT^-1
  ) / function(capture)
    tonumber(capture)
  end

function SURROUNDED_WS(pattern)
  return WS * pattern * WS
end

function JOIN(separator, argument)
  return argument * (separator * argument)^0
end

function JOIN_WS(argument) return JOIN(WS, argument) end

function JOINED(separator, ...)
  parts = {...}
  count = #parts
  assert(count > 0)
  result = parts[1]
  for index = 2, count do
    result = result * separator * parts[index]
  end
  return result
end

function JOINED_WS(...) return JOINED(WS, ...) end

local function Block(pattern)
  return JOINED_WS(P"{", pattern, P"}")
end

local function Parenthesized(pattern)
  return JOINED_WS(P"(", pattern, P")")
end

local function Statement(pattern)
  return JOINED_WS(pattern, P";")
end

-- {{{
-- TESTING ERROR HANDLING
local function expect(expected)
  -- use a match-time capture to update the state whenever the parser
  -- hits the result of this function
  return lpeg.Cmt(
    lpeg.Carg(1), -- captures the state object passed to :match(...)
    function(subject, position, state)
      state:expect(expected, position)
      return true
    end
  )
end

local function commit()
  return lpeg.Cmt(
    lpeg.Carg(1),
    function(subject, position, state)
      state:commit()
      return true
    end
  )
end
-- }}}

local function Node(node_type, pattern)
  return node.make(node_type,
    expect(node_type) * pattern * commit()
  )
end

function Multary(node_type, pattern)
  return Node(node_type,
    node.store_all("operands", pattern)
  ) / function(capture)
    assert(type(capture.operands) == "table")
    assert(capture.operands.node_type == nil)
    assert(#capture.operands > 0)
    if #capture.operands == 1 then
      return capture.operands[1]
    end
    return capture
  end
end

function UNARY_WS(operator, argument)
  return JOINED_WS(operator, argument)
end

function Unary(node_type, operator, pattern)
  return Node(node_type, UNARY_WS(
    node.store_one("operator", C(operator)),
    node.store_one("operand", pattern)
  ))
end

function INFIX_WS(operator, argument)
  return JOIN(SURROUNDED_WS(operator), argument)
end

function Infix(node_type, operator, argument)
  return Multary(node_type, INFIX_WS(operator, argument))
end


local ndtype = Node("ndtype", JOINED_WS(
  node.store_one("type", P"real" + P"integer" + P"boolean"),
  node.store_all("extents", JOIN_WS(
    JOINED_WS(P"[", C(R"09")^1 + node.value(0), P"]")
  )^-1)
))

grammar.grammar = lpeg.P{
  "start",

  start = node.make("prtcl",
    SURROUNDED_WS(
      node.store_all("statements", JOIN_WS(
        V"scheme"
      ))
    ) * EOS
  ),

  scheme = Node("scheme",
    JOINED_WS(id.WORD("scheme"),
      id.store_name("name"), Block(
      node.store_all("statements", JOIN_WS(
        V"groups" + V"global" + V"procedure"
      )^-1)
    ))
  ),

  groups = Node("groups", JOINED_WS(
    id.WORD("groups"),
      id.store_name("name"), Block(
      JOINED_WS(
        Statement(JOINED_WS(
          id.WORD("select"),
          node.store_one("select_expr", V"groups_select_expr")
        )),
        node.store_all("fields", JOIN_WS(V"groups_field")^-1)
      )
    )
  )),

  groups_select_expr = P{"expression",

    expression = V"dis_term",

    dis_term = Infix("logic_dis", P"or",  V"con_term"),
    con_term = Infix("logic_con", P"and", V"not_term"),
    not_term = Unary("logic_not", P"not", V"atom") + V"atom",

    atom =
        Node("group_selector", JOINED_WS(
          node.store_one("kind", P"type" + P"tag"),
          id.store_name("name")
        ))
      +
        SURROUNDED_WS(Parenthesized(V"expression")),
  },

  groups_field = Statement(Node("field_def", JOINED_WS(
    node.store_one("kind", P"uniform" + P"varying"),
    V"_field"
  ))),

  global = Node("global", JOINED_WS(
    id.WORD("global"), Block(
      node.store_all("fields", JOIN_WS(
        V"global_field"
      )^-1)
    )
  )),

  global_field = Statement(Node("field_def", JOINED_WS(
    node.store_one("kind", node.value("global")),
    V"_field"
  ))),

  _field = JOINED_WS(
    id.WORD("field"), id.store_name("alias"), P"=",
    node.store_one("type", V"ndtype"), id.store_name("name")
  ),

  procedure = Node("procedure", JOINED_WS(
    id.WORD("procedure"),
      id.store_name("name"), Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"compute" + V"foreach_particle" + V"solve"
      )^-1)
  ))),

  foreach_particle = Node("foreach_particle", JOINED_WS(
    id.WORD("foreach"), id.store_name("groups"),
          id.WORD("particle"), id.store_name("index"), Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"reduce" + V"compute" + V"foreach_neighbor"
      )^-1)
    )
  )),

  foreach_neighbor = Node("foreach_neighbor", JOINED_WS(
    id.WORD("foreach"), id.store_name("groups"),
          id.WORD("neighbor"), id.store_name("index"), Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"reduce" + V"compute"
      )^-1)
    ) 
  )),

  solve = Node("solve", JOINED_WS(
    id.WORD("solve"), id.store_name("solver"),
          node.store_one("type", V"ndtype"),
          id.WORD("over"), id.store_name("groups"),
          id.WORD("particle"), id.store_name("index"),
          Block(
      node.store_all("statements", JOIN_WS(
        V("setup") + V("product") + V("apply")
      )^-1)
    )
  )),

  setup = Node("solve_setup", JOINED_WS(
    id.WORD("setup"), id.store_name("name"),
          id.WORD("into"), id.store_name("into"),
          Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"compute" + V"foreach_neighbor"
      )^-1)
    )
  )),

  product = Node("solve_product", JOINED_WS(
    id.WORD("product"), id.store_name("name"),
          id.WORD("with"), id.store_name("with"),
          id.WORD("into"), id.store_name("into"),
          Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"compute" + V"foreach_neighbor"
      )^-1)
    )
  )),

  apply = Node("solve_apply", JOINED_WS(
    id.WORD("apply"), id.store_name("with"), Block(
      node.store_all("statements", JOIN_WS(
        V"local_def" + V"compute" + V"foreach_neighbor"
      )^-1)
    )
  )),

  local_def = Statement(Node("local_def", JOINED_WS(
    id.WORD("local"), id.store_name("name"),
    P":", node.store_one("type", V"ndtype"),
    P"=", node.store_one("math", V"math")
  ))),

  compute = Statement(Node("compute", JOINED_WS(
    id.WORD("compute"),
    node.store_one("target", V"math"),
    node.store_one("operator", P"=" + V"OPEQ"),
    node.store_one("source", V"math")
  ))),

  reduce = Statement(Node("reduce", JOINED_WS(
    id.WORD("reduce"),
    node.store_one("target", V"math"),
    node.store_one("operator", V"OPEQ"),
    node.store_one("source", V"math")
  ))),

  OPEQ = (S"+-*/" + P"min" + P"max") * P"=",
  
  ndtype = ndtype,

  math = P{"expression",

    ADD = P"+", SUB = P"-", MUL = P"*", DIV = P"/", NEG = P"-",

    expression = V"add_term",

    add_term = Infix("addsub", C(V"ADD" + V"SUB"), V"mul_term"),
    mul_term = Infix("muldiv", C(V"MUL" + V"DIV"), V"neg_term"),
    neg_term = Unary("neg", V"NEG", V"slice") + V"slice",

    slice =
        Node("slice", JOINED_WS(
          node.store_one("subject", V"atom"),
          P"[",
          node.store_all("indices",
            INFIX_WS(P",", C(number) + id.name())
          ),
          P"]"
        ))
      +
        V"atom",

    atom =
        Node("field_access", JOINED_WS(
          id.store_name("name"),
          P".",
          id.store_name("from")
        ))
      +
        Node("call", JOINED_WS(
            id.store_name("name"),
            node.store_one("type",
              JOINED_WS(P"<", V"ndtype", P">")^-1
            ),
            node.store_all("arguments",
              Parenthesized(INFIX_WS(P",", V"expression")^-1)
            )
        ))
      +
        Node("name_ref", id.store_name("name"))
      +
        -- TODO: extented (typed) literals
        Node("literal", node.store_one("value", number))
      +
        SURROUNDED_WS(Parenthesized(V"expression")),

    ndtype = ndtype,
  },
}

local function format_node_gv(o, tree)
  local function node_ref(tree)
    return string.format("%s_%d_%d",
      tree.node_type, tree.node_position_b, tree.node_position_e
    )
  end

  local label = string.format('%s(%d, %d)',
    tree.node_type, tree.node_position_b, tree.node_position_e
  )

  local tree_ref = node_ref(tree)

  -- attributes of the current node
  o:iput(tree_ref .. ' [shape=box, label="' .. label .. '"];'):nl()

  -- attributes of all property-nodes
  for k, prop in pairs(tree) do
    if not k:match("node_", 1) then
      local prop_ref = tree_ref .. '_' .. k

      o:iput(prop_ref .. ' [')
      if type(prop) == "table" then
        o:put('shape=invhouse, ')
        o:put('label="' .. k .. '"')
      else
        o:put('shape=cds, ')
        o:put('label="' .. k .. ': ' .. tostring(prop) .. '"')
      end
      o:put('];'):nl()

      o:iput(tree_ref .. ' -> ' .. prop_ref .. ';'):nl():nl()
    end
  end

  -- newline between attributes and the child nodes
  o:nl()

  for k, prop in pairs(tree) do
    if not k:match("node_", 1) and type(prop) == "table" then
      local prop_ref = tree_ref .. '_' .. k

      if prop.node_type == nil then
        for index, item in ipairs(prop) do
          if type(item) == "table" then
            local item_ref = node_ref(item)
            o:iput(prop_ref .. ' -> ' .. item_ref .. ';'):nl():nl()
            format_node_gv(o, item)
          else
            local item_ref = prop_ref .. '_' .. tostring(index)
            o:iput(prop_ref .. ' -> ' .. item_ref .. ';'):nl()
            o:iput(item_ref .. ' [shape=hexagon, label="' .. tostring(item) .. '"];'):nl()
          end
        end
      else
        local item_ref = node_ref(prop)
        o:iput(prop_ref .. ' -> ' .. item_ref .. ';'):nl()
        format_node_gv(o, prop)
      end
    end
  end
end

function grammar.format_gv(o, tree)
  o:iput("digraph prtcl_parse_tree {"):nl():nl()
  o:increase_indent()
  format_node_gv(o, tree)  
  o:decrease_indent()
  o:iput("}"):nl()
end

local function build_line_table(subject)
  -- find the position (offset) of each newline in the subject
  local line_table = Ct(
    lpeg.Cc(1) * (P("\n") * lpeg.Cp() + (P(1) - P("\n")))^0 * P(-1)
  ):match(subject)

  function line_table:query(position)
    local row, column = 1, 1
    for number, offset in ipairs(self) do
      if offset <= position then
        row, column = number, position - offset + 1
      else
        break
      end
    end
    return row, column
  end

  return line_table
end

function grammar.parse(subject)
  local state = { stack = {} }

  function state:expect(...) table.insert(self.stack, {...}) end
  function state:commit() table.remove(self.stack) end

  local result = grammar.grammar:match(subject, 1, state)

  if result == nil then
    local line_table = build_line_table(subject)

    local function print_expected(array)
      local count = #array

      --[[ prints the _full_ list of parsing records
      for index = count, 1, -1 do
        print(array[index][1] .. ' ' .. array[index][2])
      end
      --]]

      local last_entry = array[count]
      local row, column = line_table:query(last_entry[2])
      print(string.format('syntax error at line %d column %d', row, column))
      print('expected one of:')

      local previous = nil
      for index = #array, 1, -1 do
        local entry = array[index]
        if previous == nil or previous == entry[2] then
          print("  " .. entry[1])
        else
          break
        end
        previous = array[2]
      end
    end

    print_expected(state.stack)

    print()
    print('the following may help')
    print()

    table.sort(state.stack, function(a, b) return a[2] < b[2] end)
    print_expected(state.stack)
  end

  return result
end

return grammar
