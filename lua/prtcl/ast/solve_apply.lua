local object = require "prtcl.object"
local block = require "prtcl.ast.block"

local class = object:make_class(block, "solve_apply")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.with_name = nil
end

return class
