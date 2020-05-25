local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local class = object:make_class(node, "solve_pcg")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.type = kwargs.type

  self.groups_name = nil
  self.index_name = nil

  self.setup_rhs = collection:new{owner=self}
  self.setup_guess = collection:new{owner=self}
  self.product_precond = collection:new{owner=self}
  self.product_system = collection:new{owner=self}
  self.apply = collection:new{owner=self}
end

function class:replace(child, with)
  self.setup_rhs:replace(child, with)
  self.setup_guess:replace(child, with)
  self.product_precond:replace(child, with)
  self.product_system:replace(child, with)
  self.apply:replace(child, with)
end

return class
