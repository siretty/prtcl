local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "compute")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.target = collection:new{owner=self}
  self.operator = kwargs.operator
  self.argument = collection:new{owner=self}
end

function class:replace(child, with)
  assert(self:is_parent_of(child))
  object:class_base(class).replace(self, child, with)
  self.target:replace(child, with)
  self.argument:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.target:_yield_items()
  self.argument:_yield_items()
end

return class
