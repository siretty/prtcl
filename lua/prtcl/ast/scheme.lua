local object = require "prtcl.object"
local block = require "prtcl.ast.block"

local scheme = object:make_class(block)

function scheme:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(scheme, self, kwargs)
  self.name = kwargs.name
end

return scheme
