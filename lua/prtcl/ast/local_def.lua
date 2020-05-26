local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "local_def")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.name = kwargs.name
  self.type = kwargs.type
  self.init_expr = collection:new{owner=self}
end

function class:replace(child, with)
  object:class_base(class).replace(self, child, with)
  self.init_expr:replace(child, with)
end

function class:_yield_children()
  object:class_base(class)._yield_children(self)
  self.init_expr:_yield_items()
end

return class
