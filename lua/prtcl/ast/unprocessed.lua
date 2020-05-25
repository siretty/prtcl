local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(node)

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)
  self.data = kwargs.data
end

return class
