local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local load_value = object:make_class(node)

function load_value:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(load_value, self, kwargs)

  self.source = kwargs.source
  if self.source ~= nil then
    self.source:replace_parent(self)
  end
end

function load_value:replace(child, with)
  assert(child:is_child_of(self))
  local base = object:class_base(block)

  if self.source == child then
    self.source = with
    return base.replace(self, child, with)
  end

  return self
end

return load_value
