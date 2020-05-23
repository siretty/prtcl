local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local block = object:make_class(node)

function block:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(block, self, kwargs)
  self.statements = kwargs.statements or {}
  for _, statement in ipairs(self.statements) do
    statement:replace_parent(self)
  end
end

function block:append_statement(statement)
  assert(statement ~= nil)
  statement:replace_parent(self)
  table.insert(self.statements, statement)
  return statement
end

function block:statement_index(node)
  for index, statement in ipairs(self.statements) do
    if statement == node then
      return index
    end
  end
  return nil
end

function block:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)
  local index = self:statement_index(child)
  if index ~= nil then
    self.statements[index] = with
    base.replace(self, child, with)
  end
end

return block
