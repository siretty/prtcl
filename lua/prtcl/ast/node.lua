local object = require "prtcl.object"

local node = object:make_class()

function node:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  self._parent = kwargs.parent or nil
  return self
end

function node:parent()
  return self._parent
end

function node:is_child_of(other)
  return self._parent == other
end

function node:is_parent_of(other)
  assert(other ~= nil)
  return self == other._parent
end

function node:replace(child, with)
  assert(child ~= nil)
  child._parent = nil
  if with ~= nil then
    with._parent = self
  end
  return self
end

function node:replace_me(with)
  assert(self._parent ~= nil)
  self._parent:replace(self, with)
  return with
end

function node:replace_parent(with)
  assert(with ~= nil)
  if self._parent ~= nil then
    self._parent:replace(self, nil)
  end
  self._parent = with
  return self
end

function node:debug(log)
  log:iput("node: debug"):nl()
  log:increase_indent()
  log:iput("self = " .. tostring(self) .. ""):nl()
  log:iput("parent = " .. tostring(self._parent)):nl()
  log:decrease_indent()
end

return node
