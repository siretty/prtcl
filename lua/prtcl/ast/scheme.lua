local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "scheme")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.groups = collection:new{owner=self}
  self.global = collection:new{owner=self}
  self.procedures = collection:new{owner=self}
end

function class:replace(child, with)
  object:class_base(class).replace(self, child, with)
  self.groups:replace(child, with)
  self.global:replace(child, with)
  self.procedures:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.groups:_yield_items()
  self.global:_yield_items()
  self.procedures:_yield_items()
end

return class
