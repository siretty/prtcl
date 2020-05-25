local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"

local function format_node(printer, ast)
  local function node_ident(ast)
    return string.format("%s_%s",
      object:classnameof(ast), tostring(ast):sub(10)
    )
  end

  local function node_label(ast)
    return string.format("%s",
      object:classnameof(ast)
    )
  end

  -- build the node label from non-node / non-collection properties
  label = node_label(ast)
  for key, val in pairs(ast) do
    if  not object:isinstance(val, node) and
        not object:isinstance(val, collection) then
      label = label .. string.format('\n%s: %s',
        tostring(key), tostring(val)
      )
    end
  end

  -- print the current ast node
  printer:iput(
    node_ident(ast) .. ' [shape=box, label="' .. label .. '"];'
  ):nl()

  for key, val in pairs(ast) do
    if key == "_parent" then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val)
        .. ' [color=gray,style=dotted,constraint=false];'
      ):nl()
    elseif object:isinstance(val, node) then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val) .. ';'
      ):nl()
    elseif object:isinstance(val, collection) then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val) .. ';'
      ):nl()
    end
  end

  printer:nl()

  -- recurse into the node / collection properties
  for key, val in pairs(ast) do
    if key == "_parent" then
      -- ignore the _parent property (infinite recursion)
    elseif object:isinstance(val, node) then
      format_node(printer, val)
    elseif object:isinstance(val, collection) then
      printer:iput(
        node_ident(val) .. ' [shape=invhouse, label="' .. key .. '"];'
      ):nl()

      for _, subast in ipairs(val) do
        printer:iput(
          node_ident(val) .. ' -> ' .. node_ident(subast) .. ';'
        ):nl()
      end

      printer:nl()

      for _, subast in ipairs(val) do
        format_node(printer, subast)
      end
    end
  end
end

return function(printer, ast)
  printer:iput("digraph prtcl_ast {"):nl():nl()
  printer:increase_indent()
  format_node(printer, ast)  
  printer:decrease_indent()
  printer:iput("}"):nl()
end
