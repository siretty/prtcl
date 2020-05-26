local object = require "prtcl.object"

local class = object:make_class(nil, "ndtype")

function class:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(class, self, kwargs)

  self.type = kwargs.type
  self.extents = kwargs.extents
end

function class:tostring()
  local result = tostring(self.type)
  for _, extent in ipairs(self.extents) do
    result = result .. '['
    if extent ~= 0 then
      result = result .. tostring(extent)
    end
    result = result .. ']'
  end
  return result
end

return class
