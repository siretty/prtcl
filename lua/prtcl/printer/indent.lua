local object = require "prtcl.object"

local class = {}
local metatable = { __index = class }

function class:new(kwargs)
  if kwargs == nil then kwargs = {} end
  return object:make(metatable, {
    _indent = kwargs.initial_indent or 0,
    _indentation = kwargs.indentation or "  ",
  })
end

function class:set_indent(value)
  self._indent = value
end

function class:increase_indent(amount)
  if amount == nil then amount = 1 end
  assert(amount >= 0)
  self._indent = self._indent + amount
end

function class:decrease_indent(amount)
  if amount == nil then amount = 1 end
  assert(amount >= 0)
  self._indent = self._indent - amount
  assert(self._indent >= 0)
end

function class:put_into(printer)
  for i = 1, self._indent, 1 do
    printer:put(self._indentation)
  end
end

return class
