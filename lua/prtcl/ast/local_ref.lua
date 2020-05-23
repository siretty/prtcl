local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local local_ref = object:make_class(block)

function local_ref:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(local_ref, self, kwargs)
  self.field_ref = kwargs.field_ref
  self.value_type = kwargs.value_type
end

return local_ref
