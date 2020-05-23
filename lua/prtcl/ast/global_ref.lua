local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local global_ref = object:make_class(block)

function global_ref:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(global_ref, self, kwargs)
  self.field_ref = kwargs.field_ref
  self.index_ref = kwargs.index_ref
  self.value_type = kwargs.value_type
end

return global_ref
