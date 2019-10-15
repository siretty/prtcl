#include <catch.hpp>

#include "../format_cxx_type.hpp"

#include <prtcl/data/integral_grid.hpp>
#include <prtcl/expression/function.hpp>
#include <prtcl/scheme/basic_scheme.hpp>

#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

template <typename T, size_t N>
struct test_scheme : prtcl::basic_scheme<T, N, prtcl::tag::host> {
  void execute_impl() {
    using namespace prtcl;
    using namespace prtcl::expression;

    auto const i = active_group{};
    auto const j = passive_group{};

    auto const x = make_varying_vector_field_name("position");
    auto const v = make_varying_vector_field_name("velocity");

    auto const t = make_uniform_scalar_field_name("step size");

    auto const dot = dot_fun{};
    auto const norm_sq = norm_squared_fun{};

    auto const select = [](auto &&...) { return true; };

    this->update_neighbourhoods();

    this->for_each_pair(select, x[i] = x[i] + t[i] / 2 * v[j],
                        x[i] = x[i] + t[i] / 2 * v[j]);
    this->for_each(select, x[i] = x[i] * norm_sq(2 * x[i]) / dot(x[i], x[i]));
  }
};

template <typename T, typename... Args> auto make_array(Args &&... args) {
  return std::array<T, sizeof...(Args)>{
      static_cast<T>(std::forward<Args>(args))...};
}

template <typename T, size_t N>
std::string format_array(std::array<T, N> const &value) {
  std::ostringstream s;
  s << "(" << value[0];
  for (size_t i = 1; i < value.size(); ++i)
    s << ", " << value[i];
  s << ")";
  return s.str();
}

template <typename Group> void display_group(Group &group) {
  auto x = get_ro_access(
      get_buffer(*group.get_varying_vector("position"), prtcl::tag::host{}));

  if (auto velocity = group.get_varying_vector("velocity")) {
    auto v = get_ro_access(
        get_buffer(*group.get_varying_vector("velocity"), prtcl::tag::host{}));

    if (auto step_size = group.get_uniform_scalar_index("step size")) {
      auto n_t = *step_size;
      auto us = get_ro_access(
          get_buffer(group.get_uniform_scalars(), prtcl::tag::host{}));

      std::cout << "t = " << us.get(n_t) << "\n";
    }

    for (size_t i = 0; i < group.size(); ++i) {
      std::cout << "x[" << i << "] = " << format_array(x.get(i)) << "\n";
      std::cout << "v[" << i << "] = " << format_array(v.get(i)) << "\n";
    }
  } else {
    for (size_t i = 0; i < group.size(); ++i) {
      std::cout << "x[" << i << "] = " << format_array(x.get(i)) << "\n";
    }
  }
}

TEST_CASE("prtcl/scheme/basic_scheme/test_scheme",
          "[prtcl][scheme][basic_scheme]") {
  using namespace prtcl;
  using namespace prtcl::expression;

  using T = float;
  constexpr size_t N = 3;

  test_scheme<T, N> scheme;
  scheme.set_neighbourhood_radius(20);

  auto g_index = scheme.add_group();

  { // initialize group
    auto &g = scheme.get_group(g_index);

    g.resize(10);

    auto x = get_rw_access(
        get_buffer(g.add_varying_vector("position"), tag::host{}));
    auto v = get_rw_access(
        get_buffer(g.add_varying_vector("velocity"), tag::host{}));

    auto n_t = g.add_uniform_scalar("step size");
    auto us = get_rw_access(get_buffer(g.get_uniform_scalars(), tag::host{}));

    us.set(n_t, 1);

    for (size_t i = 0; i < g.size(); ++i) {
      x.set(i, make_array<T>(i, 0, 0));
      v.set(i, make_array<T>(i, i + 1, i + 2));
    }
  }

  display_group(scheme.get_group(0));

  std::cout << "scheme.execute() ..." << std::endl;
  scheme.execute();
  std::cout << "... done" << std::endl;

  display_group(scheme.get_group(0));

  { // check group
    auto &g = scheme.get_group(g_index);

    auto x = get_ro_access(
        get_buffer(*g.get_varying_vector("position"), tag::host{}));
    auto v = get_ro_access(
        get_buffer(*g.get_varying_vector("velocity"), tag::host{}));

    auto n_t = *g.get_uniform_scalar_index("step size");
    auto us = get_ro_access(get_buffer(g.get_uniform_scalars(), tag::host{}));

    REQUIRE(us.get(n_t) == 1);

    for (size_t i = 0; i < g.size(); ++i) {
      REQUIRE(x.get(i) == make_array<T>(4 * (45 + i), 4 * 55, 4 * 65));
      REQUIRE(v.get(i) == make_array<T>(i, i + 1, i + 2));
    }
  }
}

template <typename Group>
void display_fluid_vtk(std::string description, Group &group, std::ostream &s) {
  s << "# vtk DataFile Version 2.0\n";
  s << description << "\n";
  s << "ASCII\n";
  s << "DATASET POLYDATA\n";

  auto s_flags = s.flags();

  auto x = get_ro_access(
      get_buffer(*group.get_varying_vector("position"), prtcl::tag::host{}));
  auto v = get_ro_access(
      get_buffer(*group.get_varying_vector("velocity"), prtcl::tag::host{}));
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

  s << "SCALARS density float 1\n";
  s << "LOOKUP_TABLE default";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << rho.get(i) << "\n";

  s << "SCALARS pressure float 1\n";
  s << "LOOKUP_TABLE default";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << p.get(i) << "\n";

  s.flags(s_flags);
}

template <typename T, size_t N>
struct sesph_scheme : prtcl::basic_scheme<T, N, prtcl::tag::host> {
  void execute_impl() {
    using namespace prtcl;
    using namespace prtcl::expression;

    static auto const i = active_group{};
    static auto const j = passive_group{};

    static auto const x = make_varying_vector_field_name("position");
    static auto const v = make_varying_vector_field_name("velocity");
    static auto const a = make_varying_vector_field_name("acceleration");

    static auto const m = make_varying_scalar_field_name("mass");
    static auto const rho = make_varying_scalar_field_name("density");
    static auto const p = make_varying_scalar_field_name("pressure");
    static auto const V = make_varying_scalar_field_name("volume");

    static auto const t = make_uniform_scalar_field_name("step size");
    static auto const h = make_uniform_scalar_field_name("smoothing scale");
    static auto const rho0 = make_uniform_scalar_field_name("rest density");
    static auto const kappa =
        make_uniform_scalar_field_name("compressibility modulus");
    static auto const nu = make_uniform_scalar_field_name("viscosity");

    static auto const g =
        make_uniform_vector_field_name("gravitational acceleration");

    static auto const W = kernel_fun{};
    static auto const GradW = kernel_gradient_fun{};

    static auto const dot = dot_fun{};
    static auto const norm_sq = norm_squared_fun{};
    static auto const max = max_fun{};

    this->update_neighbourhoods();

    auto const fluid = [](auto &active, auto &&...) {
      return active.has_flag("fluid");
    };
    auto const boundary = [](auto &active, auto &&...) {
      return active.has_flag("boundary");
    };
    auto const fluid_fluid = [](auto &active, auto &passive) {
      return active.has_flag("fluid") and passive.has_flag("fluid");
    };
    auto const fluid_boundary = [](auto &active, auto &passive) {
      return active.has_flag("fluid") and passive.has_flag("boundary");
    };
    auto const boundary_boundary = [](auto &active, auto &passive) {
      return active.has_flag("boundary") and passive.has_flag("boundary");
    };

    // compute boundary volume
    this->for_each(boundary, V[i] = 0);
    this->for_each_pair(boundary_boundary, V[i] = V[i] + W(x[i] - x[j], h[i]));
    this->for_each(boundary, V[i] = 1 / V[i]);

    // compute density and pressure
    this->for_each(fluid, rho[i] = 0);
    this->for_each_pair(fluid_fluid,
                        // accumulate density
                        rho[i] = rho[i] + m[j] * W(x[i] - x[j], h[i]));
    this->for_each_pair(fluid_boundary,
                        // accumulate density
                        rho[i] =
                            rho[i] + V[j] * rho0[i] * W(x[i] - x[j], h[i]));
    this->for_each(fluid,
                   // compute pressure
                   p[i] = kappa[i] * max(rho[i] / rho0[i] - 1, 0),
                   // gravitational acceleration
                   a[i] = g[i]);

    // compute accelerations
    this->for_each_pair(
        fluid_fluid,
        // compute viscosity acceleration
        a[i] = a[i] + nu[i] * m[i] / rho[i] * dot(v[i] - v[j], x[i] - x[j]) /
                          (norm_sq(x[i] - x[j]) + 0.01 * h[i] * h[i]) *
                          GradW(x[i] - x[j], h[i]),
        // compute pressure acceleration
        a[i] = a[i] +
               m[i] * (p[i] / (rho[i] * rho[i]) + p[j] / (rho[j] * rho[j])) *
                   GradW(x[i] - x[j], h[i]));
    this->for_each_pair(fluid_boundary,
                        // compute viscosity acceleration
                        a[i] = a[i] +
                               nu[i] * m[i] / rho[i] * dot(v[i], x[i] - x[j]) /
                                   (norm_sq(x[i] - x[j]) + 0.01 * h[i] * h[i]) *
                                   GradW(x[i] - x[j], h[i]),
                        // compute pressure acceleration
                        a[i] = a[i] + m[i] * (2 * p[i] / (rho[i] * rho[i])) *
                                          GradW(x[i] - x[j], h[i]));

    // Euler-Cromer / Symplectic-Euler
    this->for_each(fluid,
                   // compute velocity
                   v[i] = v[i] + t[i] * a[i],
                   // compute position
                   x[i] = x[i] + t[i] * v[i]);
  }
};

TEST_CASE("prtcl/scheme/basic_scheme/sesph_scheme",
          "[prtcl][scheme][basic_scheme]") {
  using namespace prtcl;
  using namespace prtcl::expression;

  using T = float;
  constexpr size_t N = 3;

  T smoothing_scale = 0.025f, time_step = 0.000'01f;

  sesph_scheme<T, N> scheme;
  scheme.set_neighbourhood_radius(2 * smoothing_scale);

  auto g_index = scheme.add_group();
  auto b_index = scheme.add_group();

  { // initialize fluid
    auto &g = scheme.get_group(g_index);
    g.add_flag("fluid");

    integral_grid<N> x_grid{{10, 10, 10}};
    g.resize(x_grid.size());

    auto x = get_rw_access(
        get_buffer(g.add_varying_vector("position"), tag::host{}));
    auto v = get_rw_access(
        get_buffer(g.add_varying_vector("velocity"), tag::host{}));
    g.add_varying_vector("acceleration");

    auto m =
        get_rw_access(get_buffer(g.add_varying_scalar("mass"), tag::host{}));
    g.add_varying_scalar("density");
    g.add_varying_scalar("pressure");

    auto n_t = g.add_uniform_scalar("step size");
    auto n_h = g.add_uniform_scalar("smoothing scale");
    auto n_rho0 = g.add_uniform_scalar("rest density");
    auto n_kappa = g.add_uniform_scalar("compressibility modulus");
    auto n_nu = g.add_uniform_scalar("viscosity");

    auto n_g = g.add_uniform_vector("gravitational acceleration");

    auto us = get_rw_access(get_buffer(g.get_uniform_scalars(), tag::host{}));
    auto vs = get_rw_access(get_buffer(g.get_uniform_vectors(), tag::host{}));

    us.set(n_t, time_step);
    us.set(n_h, smoothing_scale);
    us.set(n_rho0, 1000);
    us.set(n_kappa, 100'000);
    us.set(n_nu, 0.1f);

    vs.set(n_g, {0, 10, 0});

    size_t i = 0;
    for (auto ix : x_grid) {
      auto const h = smoothing_scale;
      x.set(i, make_array<T>(h * ix[0], h * ix[1], h * ix[2]));
      v.set(i, make_array<T>(0, 0, 0));
      m.set(i, constpow(us.get(n_h), N) * us.get(n_rho0));
      ++i;
    }
  }

  { // initialize boundary
    auto &g = scheme.get_group(b_index);
    g.add_flag("boundary");

    std::array<integral_grid<N>, 3> grids = {integral_grid<N>{1, 14, 14},
                                             integral_grid<N>{14, 1, 14},
                                             integral_grid<N>{14, 14, 1}};
    size_t count = 0;
    for (auto const &grid : grids)
      count += grid.size();
    g.resize(2 * count);

    auto x = get_rw_access(
        get_buffer(g.add_varying_vector("position"), tag::host{}));

    g.add_varying_scalar("volume");

    auto n_h = g.add_uniform_scalar("smoothing scale");
    auto us = get_rw_access(get_buffer(g.get_uniform_scalars(), tag::host{}));
    us.set(n_h, smoothing_scale);

    size_t i = 0;
    for (size_t i_grid = 0; i_grid < grids.size(); ++i_grid) {
      auto const &grid = grids[i_grid];
      for (auto ix : grid) {
        auto const h = smoothing_scale;
        auto value = make_array<T>(h * ix[0] - 2 * h, h * ix[1] - 2 * h,
                                   h * ix[2] - 2 * h);
        x.set(i, value);
        value[i_grid] += 13 * h;
        x.set(i + 1, value);
        i += 2;
      }
    }
  }

  { // dump initial state
    std::fstream f{"data.1.vtk", std::fstream::trunc | std::fstream::out};
    display_fluid_vtk("fluid initial", scheme.get_group(g_index), f);
  }

  std::cout << "scheme.execute() ..." << std::endl;
  for (size_t i = 0; i < 0.1f / time_step; ++i)
    scheme.execute();
  std::cout << "... done" << std::endl;

  { // dump initial state
    std::fstream f{"data.2.vtk", std::fstream::trunc | std::fstream::out};
    display_fluid_vtk("fluid after", scheme.get_group(g_index), f);
  }
}
