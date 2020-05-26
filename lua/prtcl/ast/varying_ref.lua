local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(node, "varying_ref")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)
  -- stores a reference to the field_def node
  self._ref_name = kwargs._ref_name
  self._ref_loop = kwargs._ref_loop
  -- debugging information
  self.debug_alias = self._ref_name.alias
  self.debug_name = self._ref_name.name
  self.debug_type = self._ref_name.type
end

return class
