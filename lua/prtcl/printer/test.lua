#!/usr/bin/env lua

local sprinter = require "prtcl.printer.string"
local fprinter = require "prtcl.printer.file"

function printer_sample_code(p, prefix)
  if prefix == nil then prefix = "" end
  p:iput(prefix .. "Hey!"):nl()
  p:increase_indent()
  p:iput(prefix .. "Whazzzzzup?!"):nl()
  p:decrease_indent()
  p:iput(prefix .. "Bye!"):nl()
end

local p = sprinter:new()
p:put("prtcl.printer.string"):nl()
printer_sample_code(p, "1: ")
print(p._buffer)

local p = fprinter:new()
p:put("prtcl.printer.file (stdout)"):nl()
printer_sample_code(p, "2: ")

local p = fprinter:new{file=io.stderr}
p:put("prtcl.printer.file (stderr)"):nl()
printer_sample_code(p, "3: ")

