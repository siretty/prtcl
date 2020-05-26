local object = require "prtcl.object"

local node = object:make_class(nil)

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

-- Helper function for replace implementations.
--
-- It replaces the parent
-- - of \p child with nil and
-- - of \p with with the former parent of \p child.
function node:_replace(child, with)
  assert(child ~= nil)
  child._parent = nil
  if with ~= nil then
    with._parent = self
  end
  return self
end

-- Default implementation (noop).
--
-- This being empty allows derived classes to unconditionally call
-- their base class implementation.
function node:replace(child, with) end

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


function node:find_ancestor(predicate)
  local ancestor = self._parent
  while ancestor ~= nil do
    if predicate(ancestor) then
      return ancestor
    end
    ancestor = ancestor._parent
  end
  return nil
end

function node:ancestors(predicate)
  return coroutine.wrap(function()
    local ancestor = self:find_ancestor(predicate)
    while ancestor ~= nil do
      coroutine.yield(ancestor)
      ancestor = ancestor:find_ancestor(predicate)
    end
  end)
end


-- default implementation (noop)
function node:_yield_children() end

-- returns an iterable that generates the children
function node:children()
  return coroutine.wrap(function()
    self:_yield_children()
  end)
end


function node:debug(log)
  log:iput("node: debug"):nl()
  log:increase_indent()
  log:iput("self = " .. tostring(self) .. ""):nl()
  log:iput("parent = " .. tostring(self._parent)):nl()
  log:decrease_indent()
end

return node
