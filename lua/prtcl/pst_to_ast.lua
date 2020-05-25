local object = require "prtcl.object"
local ast = require "prtcl.ast"


local function printf(format, ...)
  print(string.format('pst_to_ast: ' .. format, ...))
end


local handlers = {}

function handlers:dispatch(pst, ...)
  --printf('dispatching %s', pst.node_type)

  local result = nil

  local handler = self[pst.node_type]
  if handler ~= nil then
    result = handler(self, pst, ...)
  end

  if result == nil then
    return ast.unprocessed:new{data=pst}
  else
    return result
  end
end


function handlers:prtcl(pst)
  local res = ast.prtcl:new{}

  for _, pst_node in ipairs(pst.statements) do
    if pst_node.node_type == "scheme" then
      res.schemes:append(self:dispatch(pst_node))
    else
      printf('prtcl: unhandled node_type=%s', pst_node.node_type)
    end
  end

  return res
end


function handlers:scheme(pst)
  local scheme = ast.scheme:new{name=pst.name}

  for _, node in ipairs(pst.statements) do
    if     node.node_type == "groups" then
      scheme.groups:append(self:dispatch(node))
    elseif node.node_type == "global" then
      scheme.global:append(self:dispatch(node))
    elseif node.node_type == "procedure" then
      scheme.procedures:append(self:dispatch(node))
    else
      printf('scheme: unhandled node_type=%s', node.node_type)
    end
  end

  return scheme
end


function handlers:groups(pst)
  local groups = ast.groups:new{name=pst.name}

  groups.select_expr:append(self:dispatch(pst.select_expr))

  for _, pst_node in ipairs(pst.fields) do
    if     pst_node.kind == "varying" then
      groups.varying_fields:append(self:dispatch(pst_node))
    elseif pst_node.kind == "uniform" then
      groups.uniform_fields:append(self:dispatch(pst_node))
    else
      -- TODO: make this an error
      printf('groups: invalid field kind: %s', node.kind)
    end
  end

  return groups
end

function handlers:global(pst)
  local groups = ast.global:new{name=pst.name}

  for _, pst_node in ipairs(pst.fields) do
    if     pst_node.kind == "global" then
      groups.global_fields:append(self:dispatch(pst_node))
    else
      -- TODO: make this an error
      printf('groups: invalid field kind: %s', node.kind)
    end
  end

  return groups
end


function handlers:group_selector(pst)
  return ast.group_selector:new(pst)
end

function handlers:field_def(pst)
  return ast.field_def:new(pst)
end


function handlers:_handle_multary(pst, operator)
  assert(#pst.operands >= 2)
  assert(#pst.operands % 2 == 0)

  -- dispatch all operands
  local operands = {}
  for _, pst_operand in ipairs(pst.operands) do
    table.insert(operands, self:dispatch(pst_operand))
  end

  -- combine the operands from left-to-right
  while #operands >= 2 do
    local bop = ast.bop:new{operator="logic_con"}
    bop.operands:append(table.remove(operands, 1))
    bop.operands:append(table.remove(operands, 1))
    table.insert(operands, 1, bop)
  end

  return operands[1]
end

function handlers:_handle_infix(pst)
  assert(#pst.operands >= 3)
  assert(#pst.operands % 2 == 1)

  -- dispatch all operands
  local operands = {}
  for index, pst_operand in ipairs(pst.operands) do
    if index % 2 == 1 then
      -- dispatch operands (indices 1, 3, 5, ...)
      table.insert(operands, self:dispatch(pst_operand))
    else
      -- do not dispatch operators (indices 2, 4, ...)
      table.insert(operands, pst_operand)
    end
  end

  -- combine the operands from left-to-right
  while #operands >= 3 do
    local bop = ast.bop:new{}
    bop.operands:append(table.remove(operands, 1))
    bop.operator = table.remove(operands, 1)
    bop.operands:append(table.remove(operands, 1))
    table.insert(operands, 1, bop)
  end

  return operands[1]
end


function handlers:logic_dis(pst)
  return self:_handle_multary(pst, "logic_dis")
end

function handlers:logic_con(pst)
  return self:_handle_multary(pst, "logic_con")
end

function handlers:logic_not(pst)
  local uop = ast.uop:new{operator="logic_not"}
  uop.operand:append(self:dispatch(pst.operand))
  return uop
end


function handlers:_handle_named_block(pst, ast_class)
  local block = ast_class:new{name=pst.name}

  for _, pst_node in ipairs(pst.statements) do
    block.statements:append(self:dispatch(pst_node))
  end

  return block
end

function handlers:procedure(pst)
  return self:_handle_named_block(pst, ast.procedure)
end

function handlers:foreach_particle(pst)
  local block = self:_handle_named_block(pst, ast.foreach_particle)
  block.groups_name = pst.groups
  block.index_name = pst.index
  return block
end

function handlers:foreach_neighbor(pst)
  local block = self:_handle_named_block(pst, ast.foreach_neighbor)
  block.groups_name = pst.groups
  block.index_name = pst.index
  return block
end


function handlers:solve(pst)
  if pst.solver == "pcg" then
    local pcg = ast.solve_pcg:new{
      type=ast.ndtype:new(pst.type)
    }

    for _, pst_node in ipairs(pst.statements) do
      if     pst_node.node_type == "solve_setup" then
        if     pst_node.name == "right_hand_side" then
          pcg.setup_rhs:append(self:dispatch(pst_node))
        elseif pst_node.name == "guess" then
          pcg.setup_guess:append(self:dispatch(pst_node))
        else
          printf('solve_pcg: unknown setup name=%s', pst_node.name)
        end
      elseif pst_node.node_type == "solve_product" then
        if     pst_node.name == "preconditioner" then
          pcg.product_precond:append(self:dispatch(pst_node))
        elseif pst_node.name == "system" then
          pcg.product_system:append(self:dispatch(pst_node))
        else
          printf('solve_pcg: unknown product name=%s', pst_node.name)
        end
      elseif pst_node.node_type == "solve_apply" then
        pcg.apply:append(self:dispatch(pst_node))
      else
        printf('solve_pcg: unknown node_type=%s', pst_node.node_type)
      end
    end

    return pcg
  else
    printf('solve: unknown solver %s', node.solver)
  end
end

function handlers:solve_setup(pst)
  local block = self:_handle_named_block(pst, ast.solve_setup)
  block.into_name = pst.into
  return block
end

function handlers:solve_product(pst)
  local block = self:_handle_named_block(pst, ast.solve_product)
  block.with_name = pst.with
  block.into_name = pst.into
  return block
end

function handlers:solve_apply(pst)
  local block = self:_handle_named_block(pst, ast.solve_apply)
  block.with_name = pst.with
  return block
end


function handlers:local_def(pst)
  local stmt = ast.local_def:new{name=pst.name, type=pst.type}
  stmt.init_expr:append(self:dispatch(pst.math))
  return stmt
end

function handlers:compute(pst)
  local stmt = ast.compute:new{operator=pst.operator}
  stmt.target:append(self:dispatch(pst.target))
  stmt.argument:append(self:dispatch(pst.argument))
  return stmt

  --[[
  local stmt = ast.store_value:new{}

  stmt.target:append(self:dispatch(pst.target))

  if pst.operator == "=" then
    stmt.source:append(self:dispatch(pst.argument))
  else
    assert(#pst.operator >= 2)
    assert(pst.operator:sub(-1) == "=")

    local oper = ast.bop:new{operator=pst.operator:sub(1, -2)}

    local load = ast.load_value:new{}
    load.source:append(self:dispatch(pst.target))

    oper.operands:append(load)
    oper.operands:append(self:dispatch(pst.argument))

    stmt.source:append(oper)
  end

  return stmt
  --]]
end

function handlers:reduce(pst)
  local stmt = ast.reduce:new{operator=pst.operator}
  stmt.target:append(self:dispatch(pst.target))
  stmt.argument:append(self:dispatch(pst.argument))
  return stmt
  --[[
  local stmt = ast.load_modify_store:new{operator=pst.operator}
  stmt.target:append(self:dispatch(pst.target))
  stmt.argument:append(self:dispatch(pst.argument))
  return stmt
  --]]
end


function handlers:addsub(pst)
  return self:_handle_infix(pst)
end

function handlers:muldiv(pst)
  return self:_handle_infix(pst)
end

function handlers:neg(pst)
  local uop = ast.uop:new{operator="-"}
  uop.operand:append(self:dispatch(pst.operand))
  return uop
end


function handlers:slice(pst)
  local expr = ast.slice:new{}
  expr.subject:append(self:dispatch(pst.subject))
  for _, pst_node in ipairs(pst.indices) do
    expr.indices:append(self:dispatch(pst_node))
  end
  return expr
end

function handlers:field_access(pst)
  return ast.name_ref:new{name=pst.name, from=pst.from}
end

function handlers:name_ref(pst)
  return ast.name_ref:new{name=pst.name, from=nil}
end

function handlers:literal(pst)
  -- TODO: set type of literal
  return ast.literal:new{type=pst.type, value=pst.value}
end

function handlers:call(pst)
  local expr = ast.call:new{name=pst.name, type=pst.type}
  for _, pst_node in ipairs(pst.arguments) do
    expr.arguments:append(self:dispatch(pst_node))
  end
  return expr
end


local function pst_to_ast(pst)
  local tree = handlers:dispatch(pst)
  return tree
end

return pst_to_ast
