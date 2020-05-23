#!/usr/bin/env lua

local node = require "prtcl.ast.node"

local log = require("prtcl.printer.file"):new()

function test_node()
  a = node:new()
  assert(a:parent() == nil)
  assert(a:is_child_of(nil))
  --a:debug(log:put("node a"):nl())

  aa = node:new{parent=a}
  assert(aa:parent() == a)
  assert(aa:is_child_of(a))
  assert(a:is_parent_of(aa))
  --aa:debug(log:put("node aa"):nl())

  ab = node:new{parent=a}
  assert(ab:parent() == a)
  assert(ab:is_child_of(a))
  assert(a:is_parent_of(ab))
  --ab:debug(log:put("node ab"):nl())

  ab_new = ab:replace_me(node:new())
  assert(ab_new:parent() == a)
  assert(ab_new:is_child_of(a))
  assert(a:is_parent_of(ab_new))
  --ab:debug(log:put("node ab"):nl())
  --ab_new:debug(log:put("node ab_new"):nl())

  ab_new:replace_me(nil)
  assert(ab_new:parent() == nil)
  assert(ab_new:is_child_of(nil))
  assert(not a:is_parent_of(ab_new))
  --ab:debug(log:put("node ab"):nl())
  --ab_new:debug(log:put("node ab_new"):nl())
end

function test_scheme_groups_a()
  local scheme = require "prtcl.ast.scheme"
  local groups = require "prtcl.ast.groups"
  local sel = require "prtcl.ast.group_selector"

  local multary = require "prtcl.ast.multary"
  local unary = require "prtcl.ast.unary"

  local load_value = require "prtcl.ast.load_value"
  local store_value = require "prtcl.ast.store_value"
  local rmw_op = require "prtcl.ast.rmw_op"

  local varying_ref = require "prtcl.ast.varying_ref"
  local uniform_ref = require "prtcl.ast.uniform_ref"
  local global_ref = require "prtcl.ast.global_ref"
  local local_ref = require "prtcl.ast.local_ref"

  local s = scheme:new{
    name="s",
    statements={
      groups:new{name="x"},
      groups:new{name="y"},
      groups:new{name="z"},
    },
  }

  assert(s.name == "s")
  assert(#s.statements == 3)
  assert(s.statements[1].is_child_of(s))
  assert(s.statements[2].is_child_of(s))
  assert(s.statements[3].is_child_of(s))

  local a = s:append_statement(groups:new{
    name="a",
    select_expr=sel:new{kind="type", test="a_s"},
    statements={
      groups:new{name="a_1"},
      groups:new{name="a_2"},
    },
  })

  assert(a.name == "a")
  assert(a.select_expr ~= nil)
  assert(a.select_expr.test == "a_s")
  assert(#a.statements == 2)
  assert(a:parent() == s)
  assert(a:is_child_of(s))
  assert(s:is_parent_of(a))
  assert(s:statement_index(a) == 4)

  local b = s:append_statement(groups:new{
    name="b",
    select_expr=sel:new{kind="tag", test="b_s"},
    statements={
      groups:new{name="b_1"},
      groups:new{name="b_2"},
    },
  })

  assert(b.name == "b")
  assert(b.select_expr ~= nil)
  assert(b.select_expr.test == "b_s")
  assert(#b.statements == 2)
  assert(b:parent() == s)
  assert(b:is_child_of(s))
  assert(s:is_parent_of(b))
  assert(s:statement_index(b) == 5)

  local c = groups:new{name="c"}

  assert(c.name == "c")
  assert(c.select_expr == nil)
  assert(#c.statements == 0)
  assert(c:parent() == nil)
  assert(not c:is_child_of(s))
  assert(not s:is_parent_of(c))
  assert(s:statement_index(c) == nil)
end

test_node()
test_scheme_groups_a()

