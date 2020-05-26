local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(node, "solving_with_ref")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)
  -- stores a reference to the solve node
  self._ref_name = kwargs._ref_name -- solve_{setup,product,apply}
  self._ref_loop = kwargs._ref_loop -- solve_pcg / foreach_neighbor
  -- debugging information
  -- ...
end

return class
