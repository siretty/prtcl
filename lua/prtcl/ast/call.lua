local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "call")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.type = kwargs.type
  self.arguments = collection:new{owner=self}
end

function class:replace(child, with)
  object:class_base(class).replace(self, child, with)
  self.arguments:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.arguments:_yield_items()
end

return class
