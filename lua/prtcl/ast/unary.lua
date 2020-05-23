local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local unary = object:make_class(node)

function unary:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(unary, self, kwargs)
  self.operator = kwargs.operator
  self.operand = kwargs.operand
  if self.operand ~= nil then
    self.operand:replace_parent(self)
  end
end

function unary:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)
  if child == self.operand then
    self.operand = with
    base.replace(self, child, with)
  end
end

return unary
