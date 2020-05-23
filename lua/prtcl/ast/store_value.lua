local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local store_value = object:make_class(node)

function store_value:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(store_value, self, kwargs)

  self.target = kwargs.target
  if self.target ~= nil then
    self.target:replace_parent(self)
  end

  self.argument = kwargs.argument
  if self.argument ~= nil then
    self.argument:replace_parent(self)
  end
end

function store_value:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)

  if self.target == child then
    self.target = with
    return base.replace(self, child, with)
  end

  if self.argument == child then
    self.argument = with
    return base.replace(self, child, with)
  end

  return self
end

return store_value
