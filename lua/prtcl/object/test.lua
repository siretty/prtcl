#!/usr/bin/env lua

local object = require "prtcl.object"

local base = object:make_class()
local derived_a = object:make_class(base)
local derived_b = object:make_class(derived_a)

function base:_init(value)
  self.value = value
end

function base:get_value()
  return self.value
end

function derived_a:_init(...)
  object:init(derived_a, self, ...)
  self.value = self.value + 1
end

function derived_b:_init(...)
  object:init(derived_b, self, ...)
  self.value = self.value + 2
end

function derived_b:get_value()
  local base = object:class_base(derived_b)
  return base.get_value(self) + 3
end

print("testing base")
local b = base:new(5)
assert(object:classof(b) == base)
assert(object:isinstance(b, base))
assert(not object:isinstance(b, derived_a))
assert(not object:isinstance(b, derived_b))
assert(b.value == 5)
assert(b:get_value() == 5)

print("testing derived_a")
local da = derived_a:new(7)
assert(object:classof(da) == derived_a)
assert(object:isinstance(da, derived_a))
assert(object:isinstance(da, base))
assert(not object:isinstance(da, derived_b))
assert(da.value == 8)
assert(da:get_value() == 8)

print("testing derived_b")
local db = derived_b:new(9)
assert(object:classof(db) == derived_b)
assert(object:isinstance(db, derived_b))
assert(object:isinstance(db, derived_a))
assert(object:isinstance(db, base))
assert(db.value == 12)
assert(db:get_value() == 15)
