#!/usr/bin/env lua

local lpeg = require "lpeg"

local P, Ct = lpeg.P, lpeg.Ct

local example_a = [==[

scheme test_scheme {
  // dynamic particles
  groups dynamic {
    select type particle and tag dynamic;

    varying field x = real[] position;
    varying field v = real[] velocity;
    varying field a = real[] acceleration;

    varying field rho = real density;

    uniform field m = real mass;
    uniform field gs = real gravity_scale;
  }

  /* non-dynamic, aka. static particles */
  groups static {
    select type particle and not tag dynamic;

    varying field x = real[] position;
  }

  global {
    // smoothing scale for all particles
    field h = real smoothing_scale;

    // gravitational acceleration
    field g = real[] gravity;
  }

  procedure compute_density {
    foreach dynamic particle i {
      compute rho.i = 0;
      foreach dynamic neighbor j {
        compute rho.i += m.j * kernel_h(x.i - x.j, h);
      }
    }
  }

  procedure apply_gravity {
    foreach dynamic particle i {
      local scaled_gravity : real = gs.i * g;
      compute a.i += scaled_gravity;
    }
  }

  procedure solve_trivial {
    solve pcg real over dynamic particle i {
      setup guess into iterate {
        compute iterate.i = 0;
        foreach dynamic neighbor j {
          compute iterate.i += m.j * iterate.j;
        }
      }

      setup right_hand_side into rhs {
        compute rhs.i = 0;
      }

      product preconditioner with iterate into result {
        compute result.i = m.i * iterate.i;
      }

      product system with iterate into result {
        compute result.i = rho.i * iterate.i;
      }

      apply iterate {
        compute rho.i = iterate.i;
      }
    }
  }
}

]==]

function test_example_a()
  local g = require "prtcl.parser.grammar"
  local h = require "prtcl.helper"

  local printer = require("prtcl.printer.file"):new()
  
  --local result = g.grammar:match(example_a)
  --
  local pst = g.parse(example_a)
  if pst ~= nil then
    --g.format_gv(printer, pst)
  end

  local ast = require("prtcl.pst_to_ast")(pst)
  if ast ~= nil then
    require("prtcl.ast.to_gv")(printer, ast)
  end
end

test_example_a()

