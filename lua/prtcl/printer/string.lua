local object = require "prtcl.object"
local base = require "prtcl.printer.base"

local sprinter = object:make_class(base)

function sprinter:_init(kwargs)
  object:init(sprinter, self, kwargs)
  if kwargs == nil then kwargs = {} end
  self._buffer = kwargs.initial_buffer or ""
  return self
end

function sprinter:put(str)
  assert(type(str) == "string")
  self._buffer = self._buffer .. str
  return self
end

function sprinter:str()
  return self._buffer
end

return sprinter
