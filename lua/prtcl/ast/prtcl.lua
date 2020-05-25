local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "prtcl")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.schemes = collection:new{owner=self}
end

function class:replace(child, with)
  self.schemes:replace(child, with)
end

return class
