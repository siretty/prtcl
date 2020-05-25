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
  local base = object:class_base(class)

  self.groups:replace(child, with)
  self.global:replace(child, with)
  self.procedures:replace(child, with)
end

return class
