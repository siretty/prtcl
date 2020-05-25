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
  local base = object:class_base(class)

  self.select_expr:replace(child, with)
  self.varying_fields:replace(child, with)
  self.uniform_fields:replace(child, with)
end

return class
