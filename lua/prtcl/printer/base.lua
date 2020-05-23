local object = require "prtcl.object"
local indent = require "prtcl.printer.indent"

local class = object:make_class()

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  self._indent_prefix = kwargs.indent_prefix or nil
  self._indent = indent:new(kwargs.indent)
  return self
end

function class:put_indent()
  if self._indent_prefix ~= nil then
    self:put(self._indent_prefix)
  end
  self._indent:put_into(self)
end

function class:iput(str)
  assert(type(str) == "string")
  self:put_indent()
  self:put(str)
  return self
end

function class:nl()
  self:put("\n")
  return self
end

function class:increase_indent()
  self._indent:increase_indent()
  return self
end

function class:decrease_indent()
  self._indent:decrease_indent()
  return self
end

return class
