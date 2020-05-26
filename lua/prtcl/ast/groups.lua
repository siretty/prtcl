local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "groups")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.select_expr = collection:new{owner=self}
  self.varying_fields = collection:new{owner=self}
  self.uniform_fields = collection:new{owner=self}
end

function class:replace(child, with)
  object:class_base(class).replace(self, child, with)
  self.select_expr:replace(child, with)
  self.varying_fields:replace(child, with)
  self.uniform_fields:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.select_expr:_yield_items()
  self.varying_fields:_yield_items()
  self.uniform_fields:_yield_items()
end

return class
