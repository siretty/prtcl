local module = {}


local lpeg = require "lpeg"

local SCOMMENT = lpeg.P("//") * (lpeg.P(1) - lpeg.P("\n"))^0
local MCOMMENT = lpeg.P("/*") * (lpeg.P(1) - lpeg.P("*/"))^0 * lpeg.P("*/")

local WHITESPACE = lpeg.S(" \n\t") + SCOMMENT + MCOMMENT
local WS0 = WHITESPACE^0
local WS1 = WHITESPACE^1


module.WS0, module.WS1 = WS0, WS1


local function sorted_keys(tbl)
  local keys = {}
  for k, _ in pairs(tbl) do table.insert(keys, k) end
  table.sort(keys)
  return keys
end
module.sorted_keys = sorted_keys


-- Print \p str with leading indent.
local function iprint(indent, str)
  print(string.rep("  ", indent) .. str)
end

-- Pretty-print helper function.
local function pprint(obj, indent)
  if obj == nil then
    iprint(indent, "nil")
  else
    if type(obj) == "table" then
      local keys = sorted_keys(obj)
      for _, k in ipairs(keys) do
        local v = obj[k]
        if type(v) ~= "table" then
          iprint(indent + 1, tostring(k) .. ": " .. tostring(v))
        else
          iprint(indent + 1, tostring(k) .. ":")
          pprint(v, indent + 2)
        end
      end
    else
      iprint(indent, tostring(obj))
    end
  end
end

-- Pretty print an object and format it in a "nice" way.
function module.pprint(obj)
  pprint(obj, 0)
end


function module:import(...)
  result = {}
  for _, name in ipairs({...}) do
    table.insert(result, self[name])
  end
  return unpack(result)
end


return module
