local object = require 'prtcl.object'
local set = require 'prtcl.object.set'

local ast = require 'prtcl.ast'

local module = {}

-- {{{ hpp_header
hpp_header = [===[
#pragma once

#include <prtcl/rt/common.hpp>

#include <prtcl/rt/basic_group.hpp>
#include <prtcl/rt/basic_model.hpp>

#include <prtcl/rt/openmp/solver/cg.hpp>

#include <prtcl/rt/log/trace.hpp>

#include <vector>

#include <omp.h>

#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
]===]
-- }}}

-- {{{ hpp_footer
hpp_footer = [===[
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
]===]
-- }}}


local function raise_error(message, node, ...)
  print('ERROR: ' .. message)
  error({message=message, node=node, args={...}}, 2)
end


-- {{{ {extents,ndtype}_template_args, ndtype_data_ref_t, ndtype_t
local function extents_template_args(extents)
  local result = ''
  for _, extent in ipairs(extents) do
    if extent == 0 then
      result = result .. ', ' .. 'N'
    else
      result = result .. ', ' .. tostring(extent)
    end
  end
  return result
end

local function ndtype_template_args(ndtype)
  local result = 'dtype::' .. ndtype.type .. extents_template_args(ndtype.extents)
  --for _, extent in ipairs(ndtype.extents) do
  --  if extent == 0 then
  --    result = result .. ', ' .. 'N'
  --  else
  --    result = result .. ', ' .. tostring(extent)
  --  end
  --end
  return result
end

local function ndtype_data_ref_t(ndtype)
  return 'ndtype_data_ref_t<' .. ndtype_template_args(ndtype) .. '>'
end

local function ndtype_t(ndtype)
  return 'ndtype_t<' .. ndtype_template_args(ndtype) .. '>'
end
-- }}}

-- {{{ field_{name,decl}, index_name, groups_data_type
local function field_name(field)
  if object:isinstance(field, ast.field_def) then
    if     field.kind == 'global' then
      return 'g_' .. field.alias
    elseif field.kind == 'uniform' then
      return 'u_' .. field.alias
    elseif field.kind == 'varying' then
      return 'v_' .. field.alias
    else
      assert(false, 'invalid field_def.kind')
    end
  elseif object:isinstance(field, ast.local_def) then
    return 'l_' .. field.name
  else
    assert(false, 'invalid field class')
  end
end

local function index_name(loop)
  if     object:isinstance(loop, ast.foreach_dimension_index) then
    return 'i_' .. loop.index_name
  elseif object:isinstance(loop, ast.foreach_particle) then
    return 'i'
  elseif object:isinstance(loop, ast.solve_pcg) then
    return 'i'
  elseif object:isinstance(loop, ast.foreach_neighbor) then
    return 'j'
  else
    raise_error('node has no index', loop)
  end
end

local function field_decl(field)
  if object:isinstance(field, ast.field_def) then
    return ndtype_data_ref_t(field.type) .. ' ' .. field_name(field)
  else
    assert(false, 'invalid field class')
  end
end

local function groups_data_type(groups)
  return 'groups_' .. groups.name .. '_data'
end
-- }}}

-- {{{ cxx_{access_modifier, operator_{b,u}op, pragma}
local function cxx_access_modifier(printer, name)
  printer:decrease_indent()
  printer:iput(name .. ':'):nl()
  printer:increase_indent()
end

local function cxx_operator_bop(name)
  if     name == 'logic_con' then
    return 'and'
  elseif name == 'logic_dis' then
    return 'or'
  elseif set:new{'+', '-', '*', '/'}:contains(name) then
    return name
  else
    assert(false, 'invalid bop operator name')
  end
end

local function cxx_operator_uop(name)
  if     name == 'logic_not' then
    return 'not'
  elseif name == '-' then
    return '-'
  else
    assert(false, 'invalid uop operator name')
  end
end

local function cxx_pragma(o, text)
  o:put('#pragma '):put(text):nl()
end
-- }}}


-- {{{ class fmt_base
local fmt_base = object:make_class(object.object, 'fmt_base')

function fmt_base:dispatch(o, node)
  local classname = object:classnameof(node)
  local method = self[classname]
  if method == nil then
    raise_error('cannot dispatch: unknown node type (' .. classname .. ')', node)
  end
  return method(self, o, node)
end
-- }}}

-- {{{ class fmt_expr
local fmt_expr = object:make_class(fmt_base, 'fmt_expr')

function fmt_expr:bop(o, node)
  o:put('(')
  self:dispatch(o, node.operands[1])
  o:put(' '):put(cxx_operator_bop(node.operator)):put(' ')
  self:dispatch(o, node.operands[2])
  o:put(')')
end

function fmt_expr:uop(o, node)
  o:put('(')
  o:put(cxx_operator_uop(node.operator))
  self:dispatch(o, node.operand[1])
  o:put(')')
end
-- }}}

-- {{{ class fmt_select_expr
local fmt_select_expr = object:make_class(fmt_expr, 'fmt_select_expr')
local the_fmt_select_expr = fmt_select_expr:new{}

function fmt_select_expr:group_selector(o, node)
  if     node.kind == 'tag' then
    o:put('(group.has_tag("' .. node.name .. '"))')
  elseif node.kind == 'type' then
    o:put('(group.get_type() == "' .. node.name .. '")')
  end
end
-- }}}

-- {{{ class fmt_math_expr
local fmt_math_expr = object:make_class(fmt_expr, 'fmt_math_expr')
local the_fmt_math_expr = fmt_math_expr:new{}

function fmt_math_expr:literal(o, node)
  o:put('static_cast<dtype_t<dtype::real>>(' .. tostring(node.value) .. ')')
end

function fmt_math_expr:index_ref(o, node)
  o:put(index_name(node._ref_name))
end

function fmt_math_expr:local_ref(o, node)
  o:put(field_name(node._ref_name))
end

function fmt_math_expr:global_ref(o, node)
  o:put('g.' .. field_name(node._ref_name) .. '[0]')
end

-- {{{ fmt_math_expr: uniform_ref
function fmt_math_expr:uniform_ref(o, node)
  local group = nil
  if     object:isinstance(node._ref_loop, ast.solve_pcg) then
    group = 'p'
  elseif object:isinstance(node._ref_loop, ast.foreach_particle) then
    group = 'p'
  elseif object:isinstance(node._ref_loop, ast.foreach_neighbor) then
    group = 'n'
  else
    raise_error('uniform_ref refers to invalid loop node', node)
  end
  o:put(group .. '.' .. field_name(node._ref_name) .. '[0]')
end
-- }}}

-- {{{ fmt_math_expr: varying_ref
function fmt_math_expr:varying_ref(o, node)
  local group, index = nil, nil
  if     object:isinstance(node._ref_loop, ast.solve_pcg) then
    group, index = 'p', 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_particle) then
    group, index = 'p', 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_neighbor) then
    group, index = 'n', 'j'
  else
    raise_error('varying_ref refers to invalid loop node', node)
  end
  o:put(group .. '.' .. field_name(node._ref_name) .. '[' .. index .. ']')
end
-- }}}

-- {{{ fmt_math_expr: solving_into_ref
function fmt_math_expr:solving_into_ref(o, node)
  local index = nil
  if     object:isinstance(node._ref_loop, ast.solve_pcg) then
    index = 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_particle) then
    index = 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_neighbor) then
    index = 'j'
  else
    raise_error('varying_ref refers to invalid loop node', node)
  end
  o:put('p_into[' .. index .. ']')
end
-- }}}

-- {{{ fmt_math_expr: solving_with_ref
function fmt_math_expr:solving_with_ref(o, node)
  local index = nil
  if     object:isinstance(node._ref_loop, ast.solve_pcg) then
    index = 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_particle) then
    index = 'i'
  elseif object:isinstance(node._ref_loop, ast.foreach_neighbor) then
    index = 'j'
  else
    raise_error('varying_ref refers to invalid loop node', node)
  end
  o:put('p_with[' .. index .. ']')
end
-- }}}

-- {{{ fmt_math_expr: call
function fmt_math_expr:call(o, node)
  o:put('o::')
  if type(node.type) == 'table' then
    o:put('template ')
  end
  o:put(node.name)
  if type(node.type) == 'table' then
    o:put('<' .. ndtype_template_args(node.type) .. '>')
  end
  o:put('(')
  for arg_index, arg in ipairs(node.arguments) do
    if arg_index > 1 then o:put(', ') end
    self:dispatch(o, arg)
  end
  o:put(')')
end
-- }}}

-- {{{ fmt_math_expr: slice
function fmt_math_expr:slice(o, n)
  if #n.indices ~= 1 then
    raise_error('must have exactly one index (multiple not implemented yet)', n)
  end

  if not object:isinstance(n.indices[1], ast.index_ref) then
    raise_error('index must refer to an foreach dimension index loop', n)
  end

  if #n.subject ~= 1 then
    raise_error('must have exactly one subject', n)
  end
  
  self:dispatch(o, n.subject[1])
  o:put('[') -- TODO: support multiple indices
  self:dispatch(o, n.indices[1])
  o:put(']')
end
-- }}}
-- }}}

-- {{{ class fmt_stmt
local fmt_stmt = object:make_class(fmt_base, 'fmt_stmt')
local the_fmt_stmt = fmt_stmt:new{}

-- {{{ fmt_stmt: local_def
function fmt_stmt:local_def(o, n)
  o:iput('// local_def ...;'):nl()
  o:iput(ndtype_t(n.type) .. ' ' .. field_name(n))
  o:put(' = ')
  the_fmt_math_expr:dispatch(o, n.init_expr[1])
  o:put(';'):nl()
end
-- }}}

-- {{{ fmt_stmt: compute
function fmt_stmt:compute(o, n)
  o:iput('// compute'):nl()
  o:iput('')
  the_fmt_math_expr:dispatch(o, n.target[1])
  
  if set:new{'=', '+=', '-=', '*=', '/='}:contains(n.operator) then
    o:put(' ' .. n.operator .. ' ')
  elseif set:new{'min=', 'max='}:contains(n.operator) then
    o:put(' = o::' .. n.operator:sub(1, 3) .. '(')
    the_fmt_math_expr:dispatch(o, n.target[1])
    o:put(', ')
  else
    raise_error('invalid compute operator', n)
  end

  the_fmt_math_expr:dispatch(o, n.argument[1])

  if set:new{'min=', 'max='}:contains(n.operator) then
    o:put(')')
  end

  o:put(';'):nl()
end
-- }}}

-- {{{ fmt_stmt: reduce
function fmt_stmt:reduce(o, n)
  o:iput('// reduce'):nl()

  local lhs = nil

  local target_ref = n.target[1]
  if object:isinstance(target_ref, ast.global_ref) then
    local target_def = target_ref._ref_name
    lhs = 'r_' .. field_name(target_def)
  else
    raise_error('cannot reduce into anything but global fields', r)
  end

  o:iput(lhs)

  if set:new{'=', '+=', '-=', '*='}:contains(n.operator) then
    o:put(' ' .. n.operator .. ' ')
  elseif set:new{'min=', 'max='}:contains(n.operator) then
    o:put(' = o::' .. n.operator:sub(1, 3) .. '(' .. lhs .. ', ')
  else
    raise_error('invalid reduce operator', n)
  end

  the_fmt_math_expr:dispatch(o, n.argument[1])

  if set:new{'min=', 'max='}:contains(n.operator) then
    o:put(')')
  end

  o:put(';'):nl()
end
-- }}}

-- {{{ fmt_stmt: foreach_dimension_index
function fmt_stmt:foreach_dimension_index(o, n)
  local description = 'foreach dimension index ' .. n.index_name

  o:iput('// ' .. description):nl()
  o:iput('for (size_t ' .. index_name(n) .. ' = 0; '
      .. index_name(n) .. ' < N; ++' .. index_name(n) .. ') {'):nl()
  o:increase_indent()

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end
  
  o:decrease_indent()
  o:iput('} // ' .. description):nl()
end
-- }}}

-- {{{ fmt_stmt: foreach_particle
function fmt_stmt:foreach_particle(o, n)
  local description = 'foreach ' .. n.groups_name .. ' particle ' .. n.index_name

  o:iput('{ // ' .. description):nl()
  o:increase_indent()

  -- find all reduce statements in this loop
  local reductions = {}
  ast.dfs(n, function(n)
    if object:isinstance(n, ast.reduce) then
      -- TODO: maybe handle reduction into slice
      if not object:isinstance(n.target[1], ast.global_ref) then
        raise_error('cannot reduce into anything but gobal fields', n)
      end
      table.insert(reductions, n)
    end
  end, nil)

  -- extra arguments for the omp parallel pragma
  local omp_parallel_extra = ''

  -- {{{ initialize reductions
  if #reductions > 0 then
    o:nl()
    o:iput('// initialize reductions'):nl()
    for _, r in ipairs(reductions) do
      --o:iput('// - ')
      --the_fmt_math_expr:dispatch(o, r.target[1])
      --o:put(' ' .. r.operator .. ' ')
      --the_fmt_math_expr:dispatch(o, r.argument[1])
      --o:nl()

      local tr = r.target[1] -- target reference

      if object:isinstance(tr, ast.global_ref) then
        local td = tr._ref_name -- target definition

        local name = 'r_' .. field_name(td)

        o:iput(ndtype_t(td.type) .. ' ' .. name)
        o:put(' = ')
        the_fmt_math_expr:dispatch(o, tr)
        o:put(';'):nl()

        local omp = 'reduction('
        if     set:new{'+=', '-=', '*='}:contains(r.operator) then
          omp = omp .. r.operator:sub(1, 1)
        elseif set:new{'max=', 'min='}:contains(r.operator) then
          omp = omp .. r.operator:sub(1, 3)
        else
          raise_error('invalid reduction operator', r)
        end
        omp = omp .. ' : ' .. name .. ')'

        omp_parallel_extra = omp_parallel_extra .. ' ' .. omp
      else
        raise_error('cannot reduce into anything but global fields', r)
      end
    end
    o:nl()
  end
  -- }}}

  cxx_pragma(o, 'omp parallel' .. omp_parallel_extra)
  o:iput('{'):nl()
  o:increase_indent()

  o:iput('PRTCL_RT_LOG_TRACE_SCOPED("foreach_particle", "p=' .. n.groups_name .. '");'):nl()
  o:nl()

  local needs_neighbors = ast.contains_instance(n, ast.foreach_neighbor)

  o:iput('auto &t = _per_thread[omp_get_thread_num()];'):nl()
  o:nl()

  -- {{{ select, resize and reserve neighbors
  if needs_neighbors then
    o:iput('// select, resize and reserve neighbor storage'):nl()
    o:iput('auto &neighbors = t.neighbors;'):nl()
    o:iput('neighbors.resize(_data.group_count);'):nl()
    o:iput('for (auto &pgn : neighbors)'):nl()
    o:increase_indent()
    o:iput('pgn.reserve(100);'):nl()
    o:decrease_indent()
    o:nl()
  end
  -- }}}

  o:iput('for (auto &p : _data.groups.' .. n.groups_name .. ') {'):nl()
  o:increase_indent()

  local omp_for_extra = ''
  if not needs_neighbors then
    omp_for_extra = omp_for_extra .. ' schedule(static)'
  end

  cxx_pragma(o, 'omp for' .. omp_for_extra)

  o:iput('for (size_t i = 0; i < p._count; ++i) {'):nl()
  o:increase_indent()

  -- {{{ cleanup and find neighbors
  if needs_neighbors then
    o:iput('// cleanup neighbor storage'):nl()
    o:iput('for (auto &pgn : neighbors)'):nl()
    o:increase_indent()
    o:iput('pgn.clear();'):nl()
    o:decrease_indent()
    o:nl()

    o:iput('// find all neighbors of (p, i)'):nl()
    o:iput('nhood.neighbors(p.index, i, [&neighbors](auto n_index, auto j) {'):nl()
    o:increase_indent()
    o:iput('neighbors[n_index].push_back(j);'):nl()
    o:decrease_indent()
    o:iput('});'):nl()
    o:nl()
  end
  -- }}}

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end

  o:decrease_indent()
  o:iput('}'):nl()

  o:decrease_indent()
  o:iput('}'):nl()

  o:decrease_indent()
  o:iput('} // omp parallel region'):nl()

  -- {{{ finalize reductions
  if #reductions > 0 then
    o:nl()
    o:iput('// finalize reductions'):nl()
    for _, r in ipairs(reductions) do
      --o:iput('// - ')
      --the_fmt_math_expr:dispatch(o, r.target[1])
      --o:put(' ' .. r.operator .. ' ')
      --the_fmt_math_expr:dispatch(o, r.argument[1])
      --o:nl()

      local tr = r.target[1] -- target reference

      if object:isinstance(tr, ast.global_ref) then
        local td = tr._ref_name -- target definition
        local name = 'r_' .. field_name(td)

        o:iput('')
        the_fmt_math_expr:dispatch(o, tr)
        o:put(' = ' .. name .. ';'):nl()
      else
        raise_error('cannot reduce into anything but global fields', r)
      end
    end
    o:nl()
  end
  -- }}}

  o:decrease_indent()
  o:iput('} // ' .. description):nl()
end
-- }}}

-- {{{ fmt_stmt: foreach_neighbor
function fmt_stmt:foreach_neighbor(o, n)
  local description = 'foreach ' .. n.groups_name .. ' neighbor ' .. n.index_name

  o:iput('{ // ' .. description):nl()
  o:increase_indent()

  --o:iput('PRTCL_RT_LOG_TRACE_SCOPED("foreach_neighbor", "p=' .. n.groups_name .. '");'):nl()
  --o:nl()
  --o:iput('size_t neighbor_count = 0;'):nl()

  if n.groups_name == '@particle@' then
    o:iput('auto &n = p;'):nl()
    o:nl()
  else
    o:iput('for (auto &n : _data.groups.' .. n.groups_name .. ') {'):nl()
    o:increase_indent()
  end

  --o:iput('neighbor_count += neighbors[n.index].size();'):nl()

  o:iput('for (auto const j : neighbors[n.index]) {'):nl()
  o:increase_indent()

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end

  o:decrease_indent()
  o:iput('}'):nl()

  if n.groups_name ~= '@particle@' then
    o:decrease_indent()
    o:iput('}'):nl()
  end

  --o:iput('PRTCL_RT_LOG_TRACE_PLOT_NUMBER("neighbor count", ')
  --o:put('static_cast<int64_t>(neighbor_count));'):nl()
  
  o:decrease_indent()
  o:iput('} // ' .. description):nl()
end
-- }}}

-- {{{ fmt_stmt: solve_pcg
function fmt_stmt:solve_pcg(o, n)
  o:iput('{ // solve_pcg ...'):nl()
  o:increase_indent()

  -- TODO non-static, possibly member of _data
  o:iput('// the solver object'):nl()
  o:iput('static prtcl::rt::openmp::cg<model_policy')
  o:put(extents_template_args(n.type.extents))
  o:put('> solver;'):nl()
  o:nl()

  -- {{{ reset per-thread storage
  o:iput('// iterate over the per-thread storage'):nl()
  o:iput('for (auto &t : _per_thread) {'):nl()
  o:increase_indent()

  o:iput('// select, resize and reserve neighbor storage'):nl()
  o:iput('auto &neighbors = t.neighbors;'):nl()
  o:iput('neighbors.resize(_data.group_count);'):nl()
  o:iput('for (auto &pgn : neighbors)'):nl()
  o:increase_indent()
  o:iput('pgn.reserve(100);'):nl()
  o:decrease_indent()

  o:decrease_indent()
  o:iput('}'):nl()
  o:nl()
  -- }}}

  o:iput('for (auto &p : _data.groups.' .. n.groups_name .. ') {'):nl()
  o:increase_indent()

  o:iput('auto setup_r = ')
  self:dispatch(o, n.setup_rhs[1])
  o:nl();

  o:iput('auto setup_g = ')
  self:dispatch(o, n.setup_guess[1])
  o:nl();

  o:iput('auto product_s = ')
  self:dispatch(o, n.product_system[1])
  o:nl();

  o:iput('auto product_p = ')
  self:dispatch(o, n.product_precond[1])
  o:nl();

  o:iput('auto apply = ')
  self:dispatch(o, n.apply[1])
  o:nl();

  o:iput('auto const iterations = solver.solve(p, setup_r, setup_g, product_s, product_p, apply, p.u_pcg_max_error[0], p.u_pcg_max_iters[0]);'):nl()
  o:nl()

  o:iput('p.u_pcg_iterations[0] = iterations;'):nl()
  o:nl()

  o:iput('prtcl::core::log::debug("scheme", "pcg solver iterations ", iterations);'):nl()

  o:decrease_indent()
  o:iput('}'):nl()
  o:nl()

  o:decrease_indent()
  o:iput('} // ... solve_pcg'):nl()
end
-- }}}

-- {{{ fmt_stmt: help_solve_part_neighbors
function fmt_stmt:help_solve_part_neighbors(o, n)
  local needs_neighbors = ast.contains_instance(n, ast.foreach_neighbor)

  -- {{{ cleanup and find neighbors
  if needs_neighbors then
    o:iput('// fetch the neighbor storage for this thread'):nl()
    o:iput('auto &neighbors = _per_thread[omp_get_thread_num()].neighbors;'):nl()
    o:nl()

    o:iput('// cleanup neighbor storage'):nl()
    o:iput('for (auto &pgn : neighbors)'):nl()
    o:increase_indent()
    o:iput('pgn.clear();'):nl()
    o:decrease_indent()
    o:nl()

    o:iput('// find all neighbors of (p, i)'):nl()
    o:iput('nhood.neighbors(p.index, i, [&neighbors](auto n_index, auto j) {'):nl()
    o:increase_indent()
    o:iput('neighbors[n_index].push_back(j);'):nl()
    o:decrease_indent()
    o:iput('});'):nl()
    o:nl()
  end
  -- }}}
end
-- }}}

-- {{{ fmt_stmt: solve_setup
function fmt_stmt:solve_setup(o, n)
  o:put('[&](auto &p, size_t i, auto &p_into) {'):nl()
  o:increase_indent()

  o:iput('// solve_setup ' .. ''):nl()
  o:nl()

  self:help_solve_part_neighbors(o, n)

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end

  o:decrease_indent()
  o:iput('};'):nl()
end
-- }}}

-- {{{ fmt_stmt: solve_product
function fmt_stmt:solve_product(o, n)
  o:put('[&](auto &p, size_t i, auto &p_into, auto &p_with) {'):nl()
  o:increase_indent()

  o:iput('// solve_product ' .. ''):nl()
  o:nl()

  self:help_solve_part_neighbors(o, n)

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end

  o:decrease_indent()
  o:iput('};'):nl()
end
-- }}}

-- {{{ fmt_stmt: solve_apply
function fmt_stmt:solve_apply(o, n)
  o:put('[&](auto &p, size_t i, auto &p_with) {'):nl()
  o:increase_indent()

  o:iput('// solve_apply ' .. ''):nl()
  o:nl()

  self:help_solve_part_neighbors(o, n)

  for stmt_index, stmt in ipairs(n.statements) do
    if stmt_index > 1 then o:nl() end
    self:dispatch(o, stmt)
  end

  o:decrease_indent()
  o:iput('};'):nl()
end
-- }}}
-- }}}


function module.generate_hpp(prtcl, printer, namespaces, scheme_name)
  local o = printer

  o:put(hpp_header)

  -- {{{ open namespaces
  for index, ns in ipairs(namespaces) do
    if index > 1 then o:put(' ') end
    o:put('namespace ' .. ns .. ' {')
  end
  o:nl()
  -- }}}

  for _, scheme in ipairs(prtcl.schemes) do
    if scheme_name == nil or scheme_name == scheme.name then
      o:put('template <typename ModelPolicy_>'):nl()
      o:put('class ' .. scheme.name .. ' {'):nl()
      o:increase_indent()

      -- {{{ type aliases
      cxx_access_modifier(o, 'public')
      o:iput('using model_policy = ModelPolicy_;'):nl()
      o:iput('using type_policy = typename model_policy::type_policy;'):nl()
      o:iput('using math_policy = typename model_policy::math_policy;'):nl()
      o:iput('using data_policy = typename model_policy::data_policy;'):nl()
      o:nl()
      o:iput('using dtype = prtcl::core::dtype;'):nl()
      o:nl()
      o:iput('template <dtype DType_> using dtype_t ='):nl()
      o:increase_indent()
      o:iput('typename type_policy::template dtype_t<DType_>;'):nl()
      o:decrease_indent()
      o:nl()
      o:iput('template <dtype DType_, size_t ...Ns_> using ndtype_t ='):nl()
      o:increase_indent()
      o:iput('typename math_policy::template ndtype_t<DType_, Ns_...>;'):nl()
      o:decrease_indent()
      o:nl()
      o:iput('template <dtype DType_, size_t ...Ns_> using ndtype_data_ref_t ='):nl()
      o:increase_indent()
      o:iput('typename data_policy::template ndtype_data_ref_t<DType_, Ns_...>;'):nl()
      o:decrease_indent()
      o:nl()
      o:iput('static constexpr size_t N = model_policy::dimensionality;'):nl()
      o:nl()
      o:iput('using model_type = prtcl::rt::basic_model<model_policy>;'):nl()
      o:iput('using group_type = prtcl::rt::basic_group<model_policy>;'):nl()
      o:nl()
      -- }}}

      -- {{{ struct global_data
      if not scheme.global:empty() then
        cxx_access_modifier(o, 'private')
        o:iput('struct global_data {'):nl()
        o:increase_indent()

        for _, field in ipairs(scheme.global[1].global_fields) do
          o:iput(field_decl(field) .. ';'):nl()
        end

        o:decrease_indent()
        o:iput('};'):nl()
        o:nl()
      end
      -- }}}

      -- {{{ struct groups_..._data
      for _, groups in ipairs(scheme.groups) do
        cxx_access_modifier(o, 'private')
        o:iput('struct ' .. groups_data_type(groups) .. ' {'):nl()
        o:increase_indent()

        o:iput('size_t _count;'):nl()
        o:iput('size_t index;'):nl()
        o:nl()

        o:iput('// uniform fields'):nl()
        for _, field in ipairs(groups.uniform_fields) do
          o:iput(field_decl(field) .. ';'):nl()
        end
        o:nl()

        o:iput('// varying fields'):nl()
        for _, field in ipairs(groups.varying_fields) do
          o:iput(field_decl(field) .. ';'):nl()
        end
        o:nl()

        o:iput('static bool selects(group_type &group) {'):nl()
        o:increase_indent()

        o:iput('return ')
        the_fmt_select_expr:dispatch(o, groups.select_expr[1])
        o:put(';'):nl()

        o:decrease_indent()
        o:iput('}'):nl()

        o:decrease_indent()
        o:iput('};'):nl()
        o:nl()
      end
      -- }}}

      -- {{{ member _data
      cxx_access_modifier(o, 'private')
      o:iput('struct {'):nl()
      o:increase_indent()

      if not scheme.global:empty() then
        o:iput('global_data global;'):nl()
        o:nl()
      end

      if not scheme.groups:empty() then
        o:iput('struct {'):nl()
        o:increase_indent()

        for _, groups in ipairs(scheme.groups) do
          o:iput('std::vector<' .. groups_data_type(groups) .. '> ' .. groups.name .. ';'):nl()
        end

        o:decrease_indent()
        o:iput('} groups;'):nl()
        o:nl()
      end

      o:iput('size_t group_count;'):nl()

      o:decrease_indent()
      o:iput('} _data;'):nl()
      o:nl()
      -- }}}

      -- {{{ member _per_thread
      cxx_access_modifier(o, 'private')
      o:iput('struct per_thread_data {'):nl()
      o:increase_indent()

      o:iput('std::vector<std::vector<size_t>> neighbors;'):nl()

      -- TODO: reductions?

      o:decrease_indent()
      o:iput('};'):nl()
      o:nl()

      o:iput('std::vector<per_thread_data> _per_thread;'):nl()
      o:nl()
      -- }}}

      -- TODO: make require obsolete!
      -- {{{ method require(model)
      cxx_access_modifier(o, 'public')
      o:iput('static void require(model_type &model) {'):nl()
      o:increase_indent()

      -- {{{ global fields
      if not scheme.global:empty() then
        o:iput('// global fields'):nl()
        for _, field in ipairs(scheme.global[1].global_fields) do
          o:iput('model.template add_global<')
          o:put(ndtype_template_args(field.type))
          o:put('>("' .. field.name .. '");'):nl()
        end
        o:nl()
      end
      -- }}}

      -- {{{ uniform and varying fields
      if not scheme.groups:empty() then
        o:iput('for (auto &group : model.groups()) {'):nl()
        o:increase_indent()

        for groups_index, groups in ipairs(scheme.groups) do
          if groups_index > 1 then o:nl() end

          o:iput('if (' .. groups_data_type(groups) .. '::selects(group)) {'):nl()
          o:increase_indent()

          if not groups.uniform_fields:empty() then
            o:iput('// uniform fields'):nl()
            for _, field in ipairs(groups.uniform_fields) do
              o:iput('group.template add_uniform<')
              o:put(ndtype_template_args(field.type))
              o:put('>("' .. field.name .. '");'):nl()
            end

            if not groups.varying_fields:empty() then o:nl() end
          end

          if not groups.varying_fields:empty() then
            o:iput('// variant fields'):nl()
            for _, field in ipairs(groups.varying_fields) do
              o:iput('group.template add_varying<')
              o:put(ndtype_template_args(field.type))
              o:put('>("' .. field.name .. '");'):nl()
            end
          end

          o:decrease_indent()
          o:iput('}'):nl()
        end

        o:decrease_indent()
        o:iput('}'):nl()
      end
      -- }}}

      o:decrease_indent()
      o:iput('}'):nl()
      o:nl()
      -- }}}

      -- {{{ method load(model)
      cxx_access_modifier(o, 'public')
      o:iput('void load(model_type &model) {'):nl()
      o:increase_indent()

      -- {{{ global fields
      if not scheme.global:empty() then
        o:iput('// global fields'):nl()
        for _, field in ipairs(scheme.global[1].global_fields) do
          o:iput('_data.global.' .. field_name(field))
          o:put(' = model.template get_global<')
          o:put(ndtype_template_args(field.type))
          o:put('>("' .. field.name .. '");'):nl()
        end
        o:nl()
      end
      -- }}}

      -- {{{ uniform and varying fields
      if not scheme.groups:empty() then
        o:iput('auto group_count = model.groups().size();'):nl()
        o:iput('_data.group_count = group_count;'):nl()
        o:nl()
        for _, groups in ipairs(scheme.groups) do
          o:iput('_data.groups.' .. groups.name .. '.clear();'):nl()
        end
        o:nl()
        o:iput('for (size_t group_index = 0; group_index < group_count; ++group_index) {'):nl()
        o:increase_indent()

        o:iput('auto &group = model.groups()[group_index];'):nl()
        o:nl();

        for groups_index, groups in ipairs(scheme.groups) do
          if groups_index > 1 then o:nl() end

          o:iput('if (' .. groups_data_type(groups) .. '::selects(group)) {'):nl()
          o:increase_indent()

          o:iput('auto &data = _data.groups.')
          o:put(groups.name)
          o:put('.emplace_back();'):nl()
          o:nl()

          o:iput('data._count = group.size();'):nl()
          o:iput('data.index = group_index;'):nl()
          o:nl()

          if not groups.uniform_fields:empty() then
            o:iput('// uniform fields'):nl()
            for _, field in ipairs(groups.uniform_fields) do
              o:iput('data.' .. field_name(field))
              o:put(' = group.template get_uniform<')
              o:put(ndtype_template_args(field.type))
              o:put('>("' .. field.name .. '");'):nl()
            end

            if not groups.varying_fields:empty() then o:nl() end
          end

          if not groups.varying_fields:empty() then
            o:iput('// varying fields'):nl()
            for _, field in ipairs(groups.varying_fields) do
              o:iput('data.' .. field_name(field))
              o:put(' = group.template get_varying<')
              o:put(ndtype_template_args(field.type))
              o:put('>("' .. field.name .. '");'):nl()
            end
          end

          o:decrease_indent()
          o:iput('}'):nl()
        end

        o:decrease_indent()
        o:iput('}'):nl()
      end
      -- }}}

      o:decrease_indent()
      o:iput('}'):nl()
      -- }}}

      -- {{{ procedures
      for _, proc in ipairs(scheme.procedures) do
        o:nl()
        cxx_access_modifier(o, 'public')
        o:iput('template <typename NHood>'):nl()
        o:iput('void ' .. proc.name .. '(NHood const &nhood) {'):nl()
        o:increase_indent()

        if not scheme.global:empty() then
          o:iput('auto &g = _data.global;'):nl()
          o:nl()
        end

        o:iput('// mathematical operations'):nl()
        o:iput('using o = typename math_policy::operations;'):nl()
        o:nl()

        o:iput('// resize per-thread storage'):nl()
        o:iput('_per_thread.resize(omp_get_max_threads());'):nl()
        o:nl()

        for stmt_index, stmt in ipairs(proc.statements) do
          if stmt_index > 1 then o:nl() end
          the_fmt_stmt:dispatch(o, stmt)
        end

        o:decrease_indent()
        o:iput('}'):nl()
      end
      -- }}}

      o:decrease_indent()
      o:put('};'):nl()
    end
  end

  -- {{{ close namespaces
  for index, ns in ipairs(namespaces) do
    if index > 1 then o:put(' ') end
    o:put('}')
  end
  o:nl()
  -- }}}

  o:put(hpp_footer)
end


return module
