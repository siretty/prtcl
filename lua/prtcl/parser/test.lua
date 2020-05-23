#!/usr/bin/env lua

local lpeg = require "lpeg"

local P, Ct = lpeg.P, lpeg.Ct

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

test_example_a()

