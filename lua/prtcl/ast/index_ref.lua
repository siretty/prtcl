local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(node, "index_ref")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)
  -- stores a reference to the local node
  self._ref_name = kwargs._ref_name
end

return class
