local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "st")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.target = collection:new{owner=self}
  self.source = collection:new{owner=self}
end

function class:replace(child, with)
  self.target:replace(child, with)
  self.source:replace(child, with)
end

return class
