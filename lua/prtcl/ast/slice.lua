local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "slice")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.subject = collection:new{owner=self}
  self.indices = collection:new{owner=self}
end

function class:replace(child, with)
  self.subject:replace(child, with)
  self.indices:replace(child, with)
end

return class
