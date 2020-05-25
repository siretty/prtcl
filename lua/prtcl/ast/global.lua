local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "global")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.global_fields = collection:new{owner=self}
end

function class:replace(child, with)
  local base = object:class_base(class)

  self.global_fields:replace(child, with)
end

return class
