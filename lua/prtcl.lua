#!/usr/bin/env lua

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

local g = require "prtcl.grammar"

configuration = {
  printer_module = "prtcl.printer.string",
  --printer_module = "prtcl.printer.file",

  printer_args = {{
    indent_prefix = "//  ",

    indent = {
      initial_indent = 0,
      indentation = string.rep(" ", 2),
    },
  }},
}


local function make_printer()
  local printer_class = require(configuration.printer_module)
  return printer_class:new(unpack(configuration.printer_args or {}))
end


path = arg[1]
if path == nil then
  path = "/home/daned/doc/code/prtcl/lua/test.prtcl"
end

f = io.open(path, "rb")
source = f:read("*a")

result = g.parse(source)
if result ~= nil then g.pretty_print(result) end

local printer = make_printer()

local cxxomp = require "prtcl.format.cxx_openmp"
cxxomp.format(result[1], printer)

if printer.str ~= nil then
  print(printer:str())
end

