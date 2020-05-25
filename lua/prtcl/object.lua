local object = {}

function object:make(metatable, instance)
  setmetatable(instance, metatable)
  return instance
end

function object:make_class(base_class)
  local class = {}

  local metatable = {
    __index = class,
  }

  --print("class", class, "metatable", metatable)

  if base_class ~= nil then
    --print("base_class", base_class)
    setmetatable(class, { __index = base_class })
  end

  function class:new(...)
    local instance = object:make(metatable, {})
    instance:_init(...)
    return instance
  end

  return class
end

-- Returns the class of an object.
function object:classof(obj)
  local metatable = getmetatable(obj)
  if metatable ~= nil then
    return metatable.__index
  else
    return nil
  end
end

-- Returns the base class of a class.
function object:class_base(class)
  return object:classof(class)
end

-- Returns true if \p class is a base class of \p obj (this includes
-- obj being an instance of \p class).
function object:isinstance(obj, class)
  assert(obj ~= nil)
  assert(class ~= nil)
  local obj_class = object:classof(obj)
  while obj_class ~= nil do
    if obj_class == class then
      return true
    end
    obj_class = object:class_base(obj_class)
  end
  return false
end

-- Call the base class _init method.
function object:init(class, obj, ...)
  local base = object:class_base(class)
  if base ~= nil and base._init ~= nil then
    --print("calling base init", base)
    base._init(obj, ...)
  end
  return obj
end

return object
