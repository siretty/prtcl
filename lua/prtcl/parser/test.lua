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
    select type particle and (not tag dynamic or tag static);

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

function prtcl_to_ast(source, printer)
  if printer == nil then
    printer = require("prtcl.printer.file"):new()
  end

  local src_to_pst = require "prtcl.src_to_pst"
  
  local pst = src_to_pst(source)
  if pst == nil then
    error('error parsing the source code', 0)
  end

  local pst_to_ast = require "prtcl.pst_to_ast"

  local ast = pst_to_ast(pst)
  if ast == nil then
    error('error transforming the pst to the ast', 0)
  end

  require("prtcl.ast.to_gv")(printer, ast)
end

function test_example_a()
  f = io.open('../share/schemes/pt16.prtcl', 'r')
  source = f:read('*a')
  prtcl_to_ast(source)
  --prtcl_to_ast(example_a)
end

test_example_a()

