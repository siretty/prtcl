local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local group_selector = object:make_class(block)

function group_selector:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(group_selector, self, kwargs)
  self.kind = kwargs.kind
  self.test = kwargs.test
end

return group_selector
