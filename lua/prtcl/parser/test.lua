#!/usr/bin/env lua

local lpeg = require "lpeg"

local P, Ct = lpeg.P, lpeg.Ct

function test_whitespace()
  local ws = require "prtcl.parser.whitespace"

  local pattern = P"a" * ws.any_node() * P"b"

  local r = Ct(pattern):match("ab")
  assert(#r == 0)

  local r = Ct(pattern):match("a b")
  assert(#r == 1)
  assert(r[1].node_type == "whitespace")
  assert(r[1].node_position_b == 2)
  assert(r[1].node_position_e == 3)
  assert(r[1].text == " ")

  local r = Ct(pattern):match("a \t b")
  assert(#r == 1)
  assert(r[1].node_type == "whitespace")
  assert(r[1].node_position_b == 2)
  assert(r[1].node_position_e == 5)
  assert(r[1].text == " \t ")

  local r = Ct(pattern):match("a \t\r\n b")
  assert(#r == 1)
  assert(r[1].node_type == "whitespace")
  assert(r[1].node_position_b == 2)
  assert(r[1].node_position_e == 7)
  assert(r[1].text == " \t\r\n ")
end

function test_identifier_whitespace()
  local node = require "prtcl.parser.node"
  local ws = require "prtcl.parser.whitespace"
  local id = require "prtcl.parser.identifier"

  local pattern = node.make("test",
    node.store_all("values",
      id.name_node() * ws.any_node() * id.name_node()
    )
  )

  local r = pattern:match("a0_c \t\r\n b1_d")
  assert(r.node_type == "test")
  assert(#r.values == 3)

  assert(r.values[1].node_type == "identifier")
  assert(r.values[1].value == "a0_c")
  assert(r.values[1].node_position_b == 1)
  assert(r.values[1].node_position_e == 5)

  assert(r.values[2].node_type == "whitespace")
  assert(r.values[2].text == " \t\r\n ")
  assert(r.values[2].node_position_b == 5)
  assert(r.values[2].node_position_e == 10)

  assert(r.values[3].node_type == "identifier")
  assert(r.values[3].value == "b1_d")
  assert(r.values[3].node_position_b == 10)
  assert(r.values[3].node_position_e == 14)

  -- check asserting that the word is a valid identifier
  assert(({pcall(function() id.WORD("a0_c") end)})[1])
  assert(not ({pcall(function() id.WORD("0a_c") end)})[1])
  assert(not ({pcall(function() id.WORD("a0_c b1_d") end)})[1])

  local pattern = node.make("test",
    node.store_all("values",
      id.word_node("a0_c") * ws.any_node() * id.word_node("b1_d")
    )
  )

  local r = pattern:match("a0_c \t\r\n b1_d")
  assert(r.node_type == "test")
  assert(#r.values == 3)

  assert(r.values[1].node_type == "identifier")
  assert(r.values[1].value == "a0_c")
  assert(r.values[1].node_position_b == 1)
  assert(r.values[1].node_position_e == 5)

  assert(r.values[2].node_type == "whitespace")
  assert(r.values[2].text == " \t\r\n ")
  assert(r.values[2].node_position_b == 5)
  assert(r.values[2].node_position_e == 10)

  assert(r.values[3].node_type == "identifier")
  assert(r.values[3].value == "b1_d")
  assert(r.values[3].node_position_b == 10)
  assert(r.values[3].node_position_e == 14)
end


local example_a = [==[

scheme scheme_a {
  // this is a single-line comment
  groups groups_a_a {
    select not (type abc);

    varying field x = real [  ] /*
      they can appear here, right in the middle of something
    */ [ 0][0 ] position;
    uniform field m = real mass;
  }

  /* this is multi-line comment */
  groups groups_a_b {
    select tag def and not type ghi;
  }

  global {
    field h = real smoothing_scale;
    field g = real[] gravity;
  }

  procedure abc {
    local tmp1 : real[][][] = h * g * ones<real[]>();
    compute tmp1[2] += zeros();
    foreach groups_a_a particle i {
      local tmp2 : real[][][1] = h * g;
      compute tmp2 -= ones();
      reduce tmp1 += zeros() * a;
      foreach groups_a_b neighbor j {
        local tmp3 : real[][2] = h * g;
        compute tmp3 *= ones();
        reduce tmp1 /= ones();
        compute x.i += x.j;
      }
      local tmp2 : real[][][1] = h * g;
      compute tmp2 -= ones();
      reduce tmp1 += zeros();
    }
    local tmp1 : real[][][] = h * g * ones<real[]>();
    compute tmp1 += zeros();
  }

}

]==]

function test_example_a()
  local g = require "prtcl.parser.grammar"
  local h = require "prtcl.helper"
  --local result = g.grammar:match(example_a)
  local result = g.parse(example_a)

  if result ~= nil then
    --print(result)
    --h.pprint(result)

    local p = require("prtcl.printer.file"):new()
    g.format_gv(p, result)
  end
end


test_whitespace()
test_identifier_whitespace()
test_example_a()

