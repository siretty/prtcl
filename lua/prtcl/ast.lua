local object = require 'prtcl.object'

local ast = {}

ast.prtcl = require "prtcl.ast.prtcl"
ast.scheme = require "prtcl.ast.scheme"

ast.groups = require "prtcl.ast.groups"
ast.global = require "prtcl.ast.global"

ast.group_selector = require "prtcl.ast.group_selector"
ast.field_def = require "prtcl.ast.field_def"

ast.procedure = require "prtcl.ast.procedure"
ast.foreach_dimension_index = require "prtcl.ast.foreach_dimension_index"
ast.foreach_particle = require "prtcl.ast.foreach_particle"
ast.foreach_neighbor = require "prtcl.ast.foreach_neighbor"

ast.solve_pcg = require "prtcl.ast.solve_pcg"
ast.solve_setup = require "prtcl.ast.solve_setup"
ast.solve_product = require "prtcl.ast.solve_product"
ast.solve_apply = require "prtcl.ast.solve_apply"

ast.local_def = require "prtcl.ast.local_def"
ast.compute = require "prtcl.ast.compute"
ast.reduce = require "prtcl.ast.reduce"

ast.load_value = require "prtcl.ast.load_value"
ast.store_value = require "prtcl.ast.store_value"
ast.load_modify_store = require "prtcl.ast.load_modify_store"

ast.slice = require "prtcl.ast.slice"
ast.literal = require "prtcl.ast.literal"
ast.call = require "prtcl.ast.call"

ast.name_ref = require "prtcl.ast.name_ref"
ast.index_ref = require "prtcl.ast.index_ref"
ast.local_ref = require "prtcl.ast.local_ref"
ast.global_ref = require "prtcl.ast.global_ref"
ast.uniform_ref = require "prtcl.ast.uniform_ref"
ast.varying_ref = require "prtcl.ast.varying_ref"
ast.solving_with_ref = require "prtcl.ast.solving_with_ref"
ast.solving_into_ref = require "prtcl.ast.solving_into_ref"

ast.uop = require "prtcl.ast.uop"
ast.bop = require "prtcl.ast.bop"

ast.block = require "prtcl.ast.block"
ast.unprocessed = require "prtcl.ast.unprocessed"


function ast.dfs(node, on_ascent, on_descent)
  for child in node:children() do
    if on_descent ~= nil then on_descent(child) end

    ast.dfs(child, on_ascent, on_descent)

    if on_ascent ~= nil then on_ascent(child) end
  end
end

function ast.contains(node, predicate)
  local not_found, error = pcall(ast.dfs, node, function(n)
    if predicate(n) then
      error(n)
    end
  end)
  return not not_found
end

function ast.contains_instance(node, class)
  return ast.contains(node, function(n)
    return object:isinstance(n, class)
  end)
end


return ast
