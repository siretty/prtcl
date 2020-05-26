local object = require "prtcl.object"
local node = require "prtcl.ast.node"
local collection = require "prtcl.ast.collection"
local ndtype = require "prtcl.ndtype"

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

      if key:sub(1, 1) == "_" then
        -- ignore keys starting with an underscore
      else
        local val_str = nil
        if object:isinstance(val, ndtype) then
          val_str = val:tostring()
        else
          val_str = tostring(val)
        end

        label = label .. string.format('\n%s: %s',
          tostring(key), val_str
        )
      end
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
    elseif key == "_ref_name" then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val)
        .. ' [color=cyan,style=dashed,constraint=false];'
      ):nl()
    elseif key == "_ref_loop" then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val)
        .. ' [color=magenta,style=dashed,constraint=false];'
      ):nl()
    elseif key == "_ref_groups" then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val)
        .. ' [color=green,style=dashed,constraint=false];'
      ):nl()
    elseif object:isinstance(val, node) then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val) .. ';'
      ):nl()
    elseif object:isinstance(val, collection) then
      printer:iput(
        node_ident(ast) .. ' -> ' .. node_ident(val) .. ' [style=bold];'
      ):nl()
    end
  end

  printer:nl()

  -- recurse into the node / collection properties
  for key, val in pairs(ast) do
    if key == "_parent" or key == "_ref_name" or key == "_ref_loop" or key == "_ref_groups" then
      -- ignore the _parent property (infinite recursion)
    elseif object:isinstance(val, node) then
      format_node(printer, val)
    elseif object:isinstance(val, collection) then
      printer:iput(
        node_ident(val) .. ' [shape=invhouse, label="' .. key .. '"];'
      ):nl()

      for subidx, subast in ipairs(val) do
        printer:iput(
          node_ident(val) .. ' -> ' .. node_ident(subast)
          .. ' [style=bold,label="' .. subidx .. '"];'
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
  printer:iput("splines=false;"):nl()
  format_node(printer, ast)  
  printer:decrease_indent()
  printer:iput("}"):nl()
end
