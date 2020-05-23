local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local uniform_ref = object:make_class(block)

function uniform_ref:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(uniform_ref, self, kwargs)
  self.field_ref = kwargs.field_ref
  self.group_ref = kwargs.group_ref
  self.index_ref = kwargs.index_ref
  self.value_type = kwargs.value_type
end

return uniform_ref
