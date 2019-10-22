#include <catch.hpp>

#include <prtcl/data/integral_grid.hpp>
#include <prtcl/expr/field.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/math/traits/host_math_traits.hpp>
#include <prtcl/scheme/host_scheme.hpp>

#include <fstream>
#include <iostream>
#include <string>

template <typename Group>
void dump_boundary_vtk(std::string, Group &, std::ostream &);

template <typename Group>
void dump_fluid_vtk(std::string, Group &, std::ostream &);

template <typename T, typename... Args> auto make_array(Args &&... args) {
  return std::array<T, sizeof...(Args)>{
      static_cast<T>(std::forward<Args>(args))...};
}

TEST_CASE("prtcl/scheme/host_scheme benchmarks",
          "[prtcl][scheme][host_scheme]") {
  using namespace prtcl;

  using math_traits = prtcl::host_math_traits<float, 3>;
  using scalar_type = typename math_traits::scalar_type;
  using vector_type = typename math_traits::vector_type;
  constexpr size_t vector_extent = math_traits::vector_extent;

  struct my_scheme : prtcl::host_scheme<my_scheme, math_traits> {
    void prepare() { this->update_neighbourhoods(); }

    void execute() {
      static auto const all = [](auto &) { return true; };

      static expr::active_group const a;

      static expr::vvector<std::string> const x{{"position"}};
      static expr::vvector<std::string> const v{{"velocity"}};
      static expr::vscalar<std::string> const s{{"scaler"}};

      this->for_each(all, x[a] += s[a] * v[a]);
    }
  };

  SECTION("scheme") {
    my_scheme scheme;

    auto gi = scheme.add_group();
    auto &gd = scheme.get_group(gi);
    gd.add_varying_vector("position");
    gd.add_varying_vector("velocity");
    gd.add_varying_scalar("scaler");

    SECTION("n = 100'000") {
      gd.resize(100'000);

      scheme.create_buffers();
      scheme.prepare();
      BENCHMARK("x += s * v") { scheme.execute(); };
      scheme.destroy_buffers();
    }
  }

  SECTION("native with prtcl accessors") {
    group_data<scalar_type, vector_extent> gd;
    gd.add_varying_vector("position");
    gd.add_varying_vector("velocity");
    gd.add_varying_scalar("scaler");

    SECTION("n = 100'000") {
      gd.resize(100'000);
      auto gb = get_buffer(gd, tag::host{});

      auto x = get_rw_access(*gb.get_varying_vector("position"));
      auto v = get_rw_access(*gb.get_varying_vector("velocity"));
      auto s = get_rw_access(*gb.get_varying_scalar("scaler"));

      BENCHMARK("x += s * v") {
#pragma omp parallel for
        for (size_t i = 0; i < gb.size(); ++i) {
          x.set<vector_type>(i, x.get<vector_type>(i) +
                                    s.get(i) * v.get<vector_type>(i));
        }
      };
    }
  }
}

TEST_CASE("prtcl/scheme/host_scheme sesph",
          "[prtcl][scheme][host_scheme][sesph]") {
  using namespace prtcl;

  using T = float;
  constexpr size_t N = 3;

  using math_traits = prtcl::host_math_traits<T, N>;

  struct sesph_scheme : prtcl::host_scheme<sesph_scheme, math_traits> {
    expr::active_group const f, b;
    expr::passive_group const f_f, f_b, b_b;

    expr::uscalar<std::string> const dt{{"time step"}};

    expr::uscalar<std::string> const m = {{"mass"}};
    expr::uscalar<std::string> const rho0 = {{"rest density"}};
    expr::uscalar<std::string> const kappa{{"compressibility"}};
    expr::uscalar<std::string> const nu{{"viscosity"}};

    expr::uvector<std::string> const g{{"gravitational acceleration"}};

    expr::vvector<std::string> const x{{"position"}};
    expr::vvector<std::string> const v{{"velocity"}};
    expr::vvector<std::string> const a{{"acceleration"}};

    expr::vscalar<std::string> const V{{"volume"}};
    expr::vscalar<std::string> const rho{{"density"}};
    expr::vscalar<std::string> const p{{"pressure"}};

    expr::call_term<tag::kernel> const W;
    expr::call_term<tag::kernel_gradient> const GradW;

    expr::call_term<tag::max> const max;
    expr::call_term<tag::dot> const dot;
    expr::call_term<tag::norm_squared> const norm_sq;

    static auto fluid() {
      return [](auto &gd) { return gd.has_flag("fluid"); };
    }

    static auto boundary() {
      return [](auto &gd) { return gd.has_flag("boundary"); };
    }

    void prepare() {
      ZoneScopedN("sesph_scheme::prepare");

      this->update_neighbourhoods();

      this->for_each(
          boundary(),
          // boundary volume
          V[b] = 10,
          this->for_each_neighbour(boundary(), V[b] += W(x[b] - x[b_b])),
          V[b] = 1 / V[b]);
    }

    void execute() {
      ZoneScopedN("sesph_scheme::execute");

      this->update_neighbourhoods();

      auto const h = this->get_smoothing_scale();

      this->for_each(
          fluid(),
          // accumulate density
          rho[f] = 0,
          this->for_each_neighbour(fluid(), //
                                   rho[f] += m[f_f] * W(x[f] - x[f_f])),
          this->for_each_neighbour(boundary(), //
                                   rho[f] +=
                                   rho0[f] * V[f_b] * W(x[f] - x[f_b])),
          // compute pressure
          p[f] = kappa[f] * max(rho[f] / rho0[f] - 1, 0));

      // compute accelerations
      this->for_each(
          fluid(), a[f] = g[f],
          this->for_each_neighbour(
              fluid(),
              // compute viscosity acceleration
              a[f] +=
              nu[f] * m[f_f] / rho[f] * dot(v[f] - v[f_f], x[f] - x[f_f]) /
              (norm_sq(x[f] - x[f_f]) + h * h / 100) * GradW(x[f] - x[f_f]),
              // compute pressure acceleration
              a[f] -=
              m[f_f] *
              (p[f] / (rho[f] * rho[f]) + p[f_f] / (rho[f_f] * rho[f_f])) *
              GradW(x[f] - x[f_f])),
          this->for_each_neighbour(
              boundary(),
              // compute viscosity acceleration
              a[f] +=
              nu[f] * rho0[f] * V[f_b] / rho[f] * dot(v[f], x[f] - x[f_b]) /
              (norm_sq(x[f] - x[f_b]) + h * h / 100) * GradW(x[f] - x[f_b]),
              // compute pressure acceleration
              a[f] -= 0.7f * rho0[f] * V[f_b] * (2 * p[f] / (rho[f] * rho[f])) *
                      GradW(x[f] - x[f_b])));

      // Euler-Cromer / Symplectic-Euler
      this->for_each(fluid(),
                     // compute velocity
                     v[f] = v[f] + dt[f] * a[f],
                     // compute position
                     x[f] = x[f] + dt[f] * v[f]);
    }
  };

  T time_step = 0.00001f;
  // T time_step = 0.01f;

  sesph_scheme scheme;
  scheme.set_smoothing_scale(0.025f);

  size_t grid_size = 100; // 45;

  auto gi_fluid = scheme.add_group();
  {
    auto &gd = scheme.get_group(gi_fluid);

    integral_grid<N> x_grid{{grid_size, grid_size, grid_size}};
    gd.resize(x_grid.size());

    gd.add_flag("fluid");

    auto x = get_rw_access(
        get_buffer(gd.add_varying_vector("position"), tag::host{}));
    auto v = get_rw_access(
        get_buffer(gd.add_varying_vector("velocity"), tag::host{}));
    gd.add_varying_vector("acceleration");

    gd.add_varying_scalar("density");
    gd.add_varying_scalar("pressure");

    auto n_dt = gd.add_uniform_scalar("time step");
    auto n_m = gd.add_uniform_scalar("mass");
    auto n_rho0 = gd.add_uniform_scalar("rest density");
    auto n_kappa = gd.add_uniform_scalar("compressibility");
    auto n_nu = gd.add_uniform_scalar("viscosity");

    auto n_g = gd.add_uniform_vector("gravitational acceleration");

    auto us = get_rw_access(get_buffer(gd.get_uniform_scalars(), tag::host{}));
    auto uv = get_rw_access(get_buffer(gd.get_uniform_vectors(), tag::host{}));

    // initialize

    us.set(n_dt, 0.00001f);
    us.set(n_rho0, 1000);
    us.set(n_kappa, 100'000'000);
    us.set(n_nu, 0.01f);
    us.set(n_m, constpow(scheme.get_smoothing_scale(), N) * us.get(n_rho0));

    uv.set(n_g, {0, -10, 0});

    size_t i = 0;
    for (auto ix : x_grid) {
      auto const h = scheme.get_smoothing_scale();
      x.set(i, make_array<T>(h * ix[0], h * ix[1], h * ix[2]));
      v.set(i, make_array<T>(0, 0, 0));
      ++i;
    }
  }

  auto gi_boundary = scheme.add_group();
  {
    auto &gd = scheme.get_group(gi_boundary);

    std::array<integral_grid<N>, 3> grids = {
        integral_grid<N>{1, grid_size + 4, grid_size + 4},
        integral_grid<N>{grid_size + 4, 1, grid_size + 4},
        integral_grid<N>{grid_size + 4, grid_size + 4, 1}};
    { // resize boundary group
      size_t count = 0;
      for (auto const &grid : grids)
        count += grid.size();
      gd.resize(2 * count);
    }

    gd.add_flag("boundary");

    auto x = get_rw_access(
        get_buffer(gd.add_varying_vector("position"), tag::host{}));

    gd.add_varying_scalar("volume");

    size_t i = 0;
    for (size_t i_grid = 0; i_grid < grids.size(); ++i_grid) {
      auto const &grid = grids[i_grid];
      for (auto ix : grid) {
        auto const h = scheme.get_smoothing_scale();
        auto value = make_array<T>(h * ix[0] - 2 * h, h * ix[1] - 2 * h,
                                   h * ix[2] - 2 * h);
        x.set(i, value);
        value[i_grid] += (grid_size + 3) * h;
        x.set(i + 1, value);
        i += 2;
      }
    }
  }

  scheme.create_buffers();

  scheme.prepare();

  { // save boundary data
    std::fstream f{"boundary.vtk", std::fstream::trunc | std::fstream::out};
    dump_boundary_vtk("boundary", scheme.get_group(gi_boundary), f);
  }

  size_t max_frame = 5;
  for (size_t frame = 0; frame <= max_frame; ++frame) {
    { // save fluid data
      std::fstream f{"fluid." + std::to_string(frame) + ".vtk",
                     std::fstream::trunc | std::fstream::out};
      dump_fluid_vtk("fluid frame " + std::to_string(frame),
                     scheme.get_group(gi_fluid), f);
    }

    if (frame == max_frame)
      break;

    std::cout << "frame " << (frame + 1) << " scheme.execute() ..."
              << std::endl;
    for (size_t i = 0; i < 0.01f / time_step; ++i)
      scheme.execute();
    std::cout << "... done" << std::endl;
  }

  scheme.destroy_buffers();
}

// dump_boundary_vtk(...) {{{

template <typename Group>
void dump_boundary_vtk(std::string description, Group &group, std::ostream &s) {
  s << "# vtk DataFile Version 2.0\n";
  s << description << "\n";
  s << "ASCII\n";
  s << "DATASET POLYDATA\n";

  auto s_flags = s.flags();

  auto x = get_ro_access(
      get_buffer(*group.get_varying_vector("position"), prtcl::tag::host{}));
  auto V = get_ro_access(
      get_buffer(*group.get_varying_scalar("volume"), prtcl::tag::host{}));

  auto format_array = [](auto const &value) {
    std::ostringstream s;
    s << value[0];
    for (size_t i = 1; i < value.size(); ++i)
      s << " " << value[i];
    return s.str();
  };

  s << "POINTS " << group.size() << " float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(x.get(i)) << "\n";

  s << "POINT_DATA " << group.size() << "\n";

  s << "SCALARS volume float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << V.get(i) << "\n";

  s.flags(s_flags);
}

// }}}

// dump_fluid_vtk(...) {{{

template <typename Group>
void dump_fluid_vtk(std::string description, Group &group, std::ostream &s) {
  s << "# vtk DataFile Version 2.0\n";
  s << description << "\n";
  s << "ASCII\n";
  s << "DATASET POLYDATA\n";

  auto s_flags = s.flags();

  auto x = get_ro_access(
      get_buffer(*group.get_varying_vector("position"), prtcl::tag::host{}));
  auto v = get_ro_access(
      get_buffer(*group.get_varying_vector("velocity"), prtcl::tag::host{}));
  auto a = get_ro_access(get_buffer(*group.get_varying_vector("acceleration"),
                                    prtcl::tag::host{}));
  auto rho = get_ro_access(
      get_buffer(*group.get_varying_scalar("density"), prtcl::tag::host{}));
  auto p = get_ro_access(
      get_buffer(*group.get_varying_scalar("pressure"), prtcl::tag::host{}));

  auto format_array = [](auto const &value) {
    std::ostringstream s;
    s << value[0];
    for (size_t i = 1; i < value.size(); ++i)
      s << " " << value[i];
    return s.str();
  };

  s << "POINTS " << group.size() << " float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(x.get(i)) << "\n";

  s << "POINT_DATA " << group.size() << "\n";

  s << "VECTORS velocity float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(v.get(i)) << "\n";

  s << "VECTORS acceleration float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(a.get(i)) << "\n";

  s << "SCALARS density float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << rho.get(i) << "\n";

  s << "SCALARS pressure float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << p.get(i) << "\n";

  s.flags(s_flags);
}

// }}}
