local object = require "prtcl.object"
local base = require "prtcl.printer.base"

local fprinter = object:make_class(base)

function fprinter:_init(kwargs)
  object:init(fprinter, self, kwargs)
  if kwargs == nil then kwargs = {} end
  self._file = kwargs.file or io.stdout
  return self
end

function fprinter:put(str)
  assert(type(str) == "string")
  self._file:write(str)
  return self
end

return fprinter
