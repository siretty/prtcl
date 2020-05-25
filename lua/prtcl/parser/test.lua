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
    select tag def and (not type ghi or not tag jkl and type mno);
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

  procedure def {
    solve pcg real[] over fluid particle f {
      setup right_hand_side into result {
        local rhs : real[] = zeros<real[]>();

        foreach fluid neighbor f_f {
          compute rhs +=
              (m.f_f / rho.f_f)
            *
              0.5 * ((tvg.f + tvg.f_f) * (x.f - x.f_f))
            *
              kernel_h(x.f - x.f_f, h);
        }

        compute result.f = (1 / pt16_one.f) * rhs;
      }

      setup guess into iterate {
        compute iterate.f = v.f;
      }

      product preconditioner with iterate into result {
        // TODO: implement preconditioner
        compute result.f = iterate.f;
      }

      product system with iterate into result {
        local tmp : real[] = zeros<real[]>();

        foreach fluid neighbor f_f {
          compute tmp -=
              (m.f_f / rho.f_f)
            *
              kernel_h(x.f - x.f_f, h)
            *
              iterate.f_f;
        }

        compute result.f = iterate.f + (1 / pt16_one.f) * tmp;
      }

      apply iterate {
        // implement fade-in of particles by applying the resulting iterate
        // after the fade was completed and the previous velocity if the
        // particle is still fading in
        //compute v.f =
        //    iterate.f * unit_step_l(0, (t - t_birth.f) - dt_fade)
        //  +
        //    v.f * unit_step_r(0, dt_fade - (t - t_birth.f));

        //compute v.f = iterate.f * unit_step_l(0, (t - t_birth.f) - dt_fade);

        // compute the viscosity acceleration
        compute a.f = (iterate.f - v.f) / dt;
      }
    }
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

  require("prtcl.pst_to_ast")(result)
end

test_example_a()

