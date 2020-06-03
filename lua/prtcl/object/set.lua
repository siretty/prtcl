local object = require 'prtcl.object'

local set = object:make_class(object.object, 'set')

function set:_init(elements)
  for _, element in ipairs(elements) do
    table.insert(self, element)
  end
end

function set:contains(value)
  for _, element in ipairs(self) do
    if element == value then
      return true
    end
  end

  return false
end

function set:contains_any(...)
  for value in ipairs({...}) do
    for _, element in ipairs(self) do
      if value == element then
        return true
      end
    end
  end

  return false
end

return set
