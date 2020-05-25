local object = require "prtcl.object"
local node = require "prtcl.ast.node"

local class = object:make_class(nil, "collection")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  assert(kwargs.owner ~= nil)
  self.owner = kwargs.owner
end

function class:append(item)
  assert(item ~= nil)
  item:replace_parent(self.owner)
  table.insert(self, item)
end

function class:find(obj)
  for index, item in ipairs(self) do
    if item == obj then
      return index
    end
  end
  return nil
end

function class:replace(child, with)
  assert(child:is_child_of(self.owner))
  local owner_base = object:class_base(object:classof(self.owner))

  local index = self:find(child)
  if index ~= nil then
    self[index] = with
    owner_base.replace(self.owner, child, with)
  end
end

return class
