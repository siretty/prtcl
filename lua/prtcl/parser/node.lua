local node = {}

local lpeg = require "lpeg"

function node.store_one(name, pattern)
  return lpeg.Cg(
    pattern, name
  )
end

function node.store_all(name, pattern)
  return lpeg.Cg(
    lpeg.Ct(pattern), name
  )
end

function node.ignore(pattern)
  return pattern / function(...) end
end

function node.value(value)
  return lpeg.Cc(value)
end

function node.make(node_type, pattern)
  return lpeg.Ct(
    node.store_one("node_type", node.value(node_type))
    *
    node.store_one("node_position_b", lpeg.Cp())
    *
    pattern
    *
    node.store_one("node_position_e", lpeg.Cp())
  )
end

return node
