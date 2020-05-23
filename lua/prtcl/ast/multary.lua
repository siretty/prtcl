local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local multary = object:make_class(node)

function multary:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(multary, self, kwargs)
  self.operator = kwargs.operator
  self.operands = kwargs.operands or {}
  for _, operand in ipairs(self.operands) do
    operand:replace_parent(self)
  end
end

function multary:append_operand(operand)
  assert(operand ~= nil)
  operand:replace_parent(self)
  table.insert(self.operands, operand)
  return operand
end

function multary:operand_index(node)
  for index, operand in ipairs(self.operands) do
    if operand == node then
      return index
    end
  end
  return nil
end

function multary:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)
  local index = self:operand_index(child)
  if index ~= nil then
    self.operands[index] = with
    base.replace(self, child, with)
  end
end

return multary
