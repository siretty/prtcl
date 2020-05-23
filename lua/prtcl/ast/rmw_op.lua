local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local rmw_op = object:make_class(node)

function rmw_op:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(rmw_op, self, kwargs)

  self.target = kwargs.target
  if self.target ~= nil then
    self.target:replace_parent(self)
  end

  self.argument = kwargs.argument
  if self.argument ~= nil then
    self.argument:replace_parent(self)
  end

  self.operator = kwargs.operator
end

function rmw_op:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)

  if self.target == child then
    self.target = with
    return base.replace(self, child, with)
  end

  if self.argument == child then
    self.argument = with
    return base.replace(self, child, with)
  end

  return self
end

return rmw_op
