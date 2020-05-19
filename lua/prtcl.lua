#!/usr/bin/env lua

local g = require "prtcl.grammar"

--[[
local parse_state = {
  last_position = nil,
  last_expected = nil,
}

function tprint (tbl, indent)
  if type(tbl) ~= "table" then print("" .. (tbl or "nil")) ; return end
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      tprint(v, indent+1)
    elseif type(v) == 'boolean' then
      print(formatting .. tostring(v))      
    else
      print(formatting .. v)
    end
  end
end

subject = [==[
uniform field rho0 = real [4][5 ][]rest_density;
uniform field rho0 =real[4 ] [ 5 ][   ]  rest_density;
uniform field rho0 = real rest_density;
]==]

result = {g.field:match(subject, 1, parse_state)}
tprint(result)

if parse_state.last_expected ~= nil then
  print("expected '" .. parse_state.last_expected .. "' at position " .. parse_state.last_position)
end


subject = [==[
select not type fluid and not (tag dynamic or not type fluid) and not tag static;
select not type fluid and not tag dynamic or not type fluid and not tag static;
]==]

result = {g.sel:match(subject, 1, parse_state)}
tprint(result)

if parse_state.last_expected ~= nil then
  print("expected '" .. parse_state.last_expected .. "' at position " .. parse_state.last_position)
end

--]]

path = arg[1]
if path == nil then
  path = "/home/daned/doc/code/prtcl/lua/test.prtcl"
end

f = io.open(path, "rb")
source = f:read("*a")

result = g.parse(source)
if result ~= nil then g.pretty_print(result) end


function make_printer()
  local printer = {
    buffer = "",
    indent = 0,
    indentation = "  "
  }

  function printer:put(str)
    assert(type(str) == "string")
    self.buffer = self.buffer .. str
    return self
  end

  function printer:iput(str)
    self.buffer = self.buffer .. string.rep(self.indentation, self.indent)
    return self:put(str)
  end

  function printer:nl()
    self.buffer = self.buffer .. "\n"
    return self
  end

  function printer:increase_indent()
    self.indent = self.indent + 1
    return self
  end

  function printer:decrease_indent()
    self.indent = self.indent - 1
    assert(self.indent >= 0)
    return self
  end

  return printer
end

local printer = make_printer()
local cxxomp = require "prtcl.format.cxx_openmp"
cxxomp.format(result[1], printer)
print(printer.buffer)

