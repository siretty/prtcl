local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "field_def")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.kind = kwargs.kind
  self.name = kwargs.name
  self.type = kwargs.type
  self.alias = kwargs.alias
end

return class
