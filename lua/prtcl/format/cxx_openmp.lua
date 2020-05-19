local module = {}


local function ndtype_template_args(obj)
  assert(obj._type == "ndtype")
  local result = "dtype::" .. obj.type
  for _, v in ipairs(obj.extents) do
    if v == 0 then
      v = "N"
    end
    result = result .. ", " .. v
  end
  return result
end


local fmt = {}

function fmt:field_access(obj, output)
  output:put(obj.name .. "[" .. obj.from .. "]")
end

function fmt:call(obj, output)
  output:put("o::")
  if obj.type ~= "" then
    output:put("template ")
  end
  output:put(obj.name)
  if obj.type ~= "" then
    output:put("<" .. ndtype_template_args(obj.type) .. ">")
  end
  output:put("(")
  for i, argument in ipairs(obj.arguments) do
    if i > 1 then output:put(", ") end
    self:format(argument, output)
  end
  output:put(")")
end

function fmt:ident(obj, output)
  output:put(obj.name)
end

function fmt:number(obj, output)
  output:put(obj.value)
end

function fmt:slice(obj, output)
  self:format(obj.subject, output)
  output:put("[")
  for i, index in ipairs(obj.indices) do
    if i > 1 then output:put(", ") end
    output:put(index)
  end
  output:put("]")
end

function fmt:neg(obj, output)
  output:put("-")
  self:format(obj.operand, output)
end

function fmt:infix(obj, output)
  for i, item in ipairs(obj.operands) do
    if i % 2 == 1 then
      -- odd indices are operands
      self:format(item, output)
    else
      -- even indices are operators
      output:put(" " .. item .. " ")
    end
  end
end


function fmt:scheme(obj, output)
  output:iput("scheme " .. obj.name .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    if i > 1 then output:nl() end
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:groups(obj, output)
  output:iput("groups " .. obj.name .. " {"):nl()
  output:increase_indent()

  output:iput("select ")
  self:format(obj.select, output)
  output:put(";"):nl():nl()

  for _, field in ipairs(obj.fields) do
    self:format(field, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end


-- {{{ fmt:... groups select
function fmt:select_atom(obj, output)
  output:put(obj.kind .. " " .. obj.name)
end

function fmt:select_not(obj, output)
  output:put("not ")
  self:format(obj.operand, output)
end

function fmt:select_con(obj, output)
  for i, operand in ipairs(obj.operands) do
    if i > 1 then output:put(" and ") end
    self:format(operand, output)
  end
end

function fmt:select_dis(obj, output)
  for i, operand in ipairs(obj.operands) do
    if i > 1 then output:put(" or ") end
    self:format(operand, output)
  end
end
-- }}}


function fmt:field(obj, output)
  output:iput("field "
  .. obj.name .. " "
  .. obj.alias .. " "
  .. obj.kind .. " "
  .. ndtype_template_args(obj.type)):put(";"):nl()
end


function fmt:global(obj, output)
  output:iput("global {"):nl()
  output:increase_indent()

  for _, field in ipairs(obj.fields) do
    self:format(field, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:procedure(obj, output)
  output:iput("procedure " .. obj.name .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end


-- {{{ format: local, reduce, compute
function fmt:local_(obj, output)
  output:iput("local " .. obj.name .. " : " .. ndtype_template_args(obj.type) .. " = ")
  self:format(obj.math, output)
  output:put(";"):nl();
end

function fmt:reduce(obj, output)
  output:iput("reduce ");
  self:format(obj.into, output)
  output:put(" " .. obj.operator .. " ")
  self:format(obj.math, output)
  output:put(";"):nl()
end

function fmt:compute(obj, output)
  output:iput("compute ");
  self:format(obj.into, output)
  output:put(" " .. obj.operator .. " ")
  self:format(obj.math, output)
  output:put(";"):nl()
end
-- }}}

-- {{{ format: foreach ... particle/neighbor ... loops
function fmt:foreach_particle(obj, output)
  output:iput("foreach " .. obj.groups .. " particle " .. obj.index .. ") {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:foreach_neighbor(obj, output)
  output:iput("foreach " .. obj.groups .. " neighbor " .. obj.index .. ") {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end
-- }}}

-- {{{ format: solve blocks
function fmt:solve(obj, output)
  output:iput("solve " .. obj.solver .. " " .. ndtype_template_args(obj.type) .. " over " .. obj.groups .. " particle " .. obj.index .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    if i > 1 then output:nl() end
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:solve_setup(obj, output)
  output:iput("setup " .. obj.name .. " into " .. obj.into .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:solve_product(obj, output)
  output:iput("product " .. obj.name .. " with " .. obj.with .. " into " .. obj.into .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end

function fmt:solve_apply(obj, output)
  output:iput("apply " .. obj.with .. " {"):nl()
  output:increase_indent()

  for i, statement in ipairs(obj.statements) do
    self:format(statement, output)
  end

  output:decrease_indent()
  output:iput("}"):nl()
end
-- }}}


function fmt:format(obj, output)
  assert(type(obj) == "table")
  assert(obj._type ~= nil)
  local method = self[obj._type]
  if method == nil then
    output:put("[[_type " .. obj._type .. " not implemented]]"):nl()
  else
    self[obj._type](self, obj, output)
  end
end


function module.format(obj, output)
  assert(type(obj) == "table")
  assert(obj._type ~= nil)
  fmt:format(obj, output)
end


return module
