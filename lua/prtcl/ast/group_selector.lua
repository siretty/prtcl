local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(node, "group_selector")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.kind = kwargs.kind
  self.name = kwargs.name
end

return class
