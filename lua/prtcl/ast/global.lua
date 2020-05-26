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
  object:class_base(class).replace(self, child, with)
  self.global_fields:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.global_fields:_yield_items()
end

return class
