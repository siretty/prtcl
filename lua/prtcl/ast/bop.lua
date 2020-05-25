local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "bop")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)
  
  self.operator = kwargs.operator
  self.operands = collection:new{owner=self}
end

function class:replace(child, with)
  assert(child:is_child_of(self))
  self.operands:replace(child, with)
end

return class
