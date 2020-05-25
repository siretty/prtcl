local object = require "prtcl.object"
local block = require "prtcl.ast.block"

local class = object:make_class(block, "foreach_neighbor")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.groups_name = nil
  self.groups_ref = nil
  self.index_name = nil
  self.index_ref = nil
end

return class
