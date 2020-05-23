local identifier = {}

local lpeg = require "lpeg"

local node = require "prtcl.parser.node"

local FIRST = lpeg.R("az", "AZ")
local LATER = lpeg.R("az", "AZ", "09") + lpeg.S"_"

local WORD = FIRST * LATER^0 * -LATER

function identifier.NAME()
  return WORD
end

function identifier.name()
  return lpeg.C(identifier.NAME())
end

function identifier.store_name(name)
  return node.store_one(name, identifier.name())
end

function identifier.name_node(node_type)
  if node_type == nil then node_type = "identifier" end
  return node.make(node_type, identifier.store_name("value"))
end

function identifier.WORD(text)
  -- ensure that text is a valid identifier
  assert((WORD * -1):match(text) == #text + 1)
  return lpeg.P(text)
end

function identifier.word(text)
  return lpeg.C(identifier.WORD(text))
end

function identifier.store_word(name, text)
  return node.store_one(name, identifier.word(text))
end

function identifier.word_node(text, node_type)
  if node_type == nil then node_type = "identifier" end
  return node.make(node_type, identifier.store_word("value", text))
end

return identifier
