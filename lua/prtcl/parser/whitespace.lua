local whitespace = {}

local lpeg = require "lpeg"
local node = require "prtcl.parser.node"

local WS = lpeg.S" \t\r\n" -- TODO: think about newline handling

function whitespace.ANY()
  return WS^0
end

function whitespace.SOME()
  return WS^1
end

function whitespace.any()
  return lpeg.C(whitespace.ANY())
end

function whitespace.any_node(name)
  return node.make("whitespace",
    node.store_one("text", whitespace.ANY())
  ) / function(capture)
    assert(capture ~= nil)
    if capture.text == "" then
      return
    else
      return capture
    end
  end
end

return whitespace
