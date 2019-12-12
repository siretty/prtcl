#include <catch.hpp>

#include <prtcl/data/integral_grid.hpp>
#include <prtcl/scheme/openmp_scheme.hpp>
#include <prtcl/scheme/sesph.hpp>

#include <fstream>

template <typename Group>
void dump_boundary_vtk(std::string, Group &, std::ostream &);

template <typename Group>
void dump_fluid_vtk(std::string, Group &, std::ostream &);

TEST_CASE("prtcl/scheme/openmp_scheme", "[prtcl][scheme][openmp_scheme]") {
  namespace tag = prtcl::tag;
  constexpr tag::kind::uniform uniform;
  constexpr tag::kind::varying varying;
  constexpr tag::type::scalar scalar;
  constexpr tag::type::vector vector;

  auto const sesph_boundary_volume = prtcl::scheme::sesph_boundary_volume();
  auto const sesph_density_pressure = prtcl::scheme::sesph_density_pressure();
  auto const sesph_acceleration = prtcl::scheme::sesph_acceleration();
  auto const sesph_symplectic_euler = prtcl::scheme::sesph_symplectic_euler();

  using Scalar = float;
  constexpr size_t N = 3;

  prtcl::data::scheme<Scalar, N> data;
  auto &f = data.add_group("the_fluid").add_flag("fluid");
  auto &b = data.add_group("the_boundary").add_flag("boundary");

  auto scheme = prtcl::scheme::make_openmp_scheme(
      data, boost::hana::make_tuple(sesph_boundary_volume),
      boost::hana::make_tuple(sesph_density_pressure, sesph_acceleration,
                              sesph_symplectic_euler));

  data.get(scalar)["smoothing_scale"] = 0.025f;
  data.get(scalar)["time_step"] = 0.00001f;
  data.get(vector)["gravity"].setZero();
  data.get(vector)["gravity"][1] = -9.81f;

  f.get(uniform, scalar)["rest_density"] = 1000;
  f.get(uniform, scalar)["compressibility"] = 10'000'000;
  f.get(uniform, scalar)["viscosity"] = 0.01f;
  f.get(uniform, scalar)["mass"] =
      prtcl::constpow(data.get(scalar)["smoothing_scale"], N) *
      f.get(uniform, scalar)["rest_density"];

  size_t grid_size = 4; // 45;

  { // initialize fluid position and velocity
    auto h = data.get(scalar)["smoothing_scale"];

    prtcl::integral_grid<N> x_grid;
    x_grid.extents.fill(grid_size);

    f.resize(x_grid.size());

    size_t i = 0;
    auto &x = f.get(varying, vector)["position"];
    auto &v = f.get(varying, vector)["velocity"];
    for (auto ix : x_grid) {
      x[i][0] = h * ix[0];
      x[i][1] = h * ix[1];
      x[i][2] = h * ix[2];
      v[i].setZero();
      ++i;
    }
  }

  { // initialize boundary position
    auto h = data.get(scalar)["smoothing_scale"];

    std::array<prtcl::integral_grid<N>, 3> grids = {
        prtcl::integral_grid<N>{1, grid_size + 4, grid_size + 4},
        prtcl::integral_grid<N>{grid_size + 4, 1, grid_size + 4},
        prtcl::integral_grid<N>{grid_size + 4, grid_size + 4, 1}};

    { // resize boundary group
      size_t count = 0;
      for (auto const &grid : grids)
        count += grid.size();
      b.resize(2 * count);
    }

    auto &x = b.get(varying, vector)["position"];

    size_t i = 0;
    for (size_t i_grid = 0; i_grid < grids.size(); ++i_grid) {
      auto const &grid = grids[i_grid];
      for (auto ix : grid) {
        x[i][0] = h * ix[0] - 2 * h;
        x[i][1] = h * ix[1] - 2 * h;
        x[i][2] = h * ix[2] - 2 * h;

        x[i + 1][0] = x[i][0] + (grid_size + 3) * h;
        x[i + 1][1] = x[i][1] + (grid_size + 3) * h;
        x[i + 1][2] = x[i][2] + (grid_size + 3) * h;

        i += 2;
      }
    }
  }

  auto time_step = data.get(scalar)["time_step"];

  scheme.prepare();

  { // save boundary data
    std::fstream file{"boundary.vtk", std::fstream::trunc | std::fstream::out};
    dump_boundary_vtk("boundary", b, file);
  }

  size_t max_frame = 5;
  for (size_t frame = 0; frame <= max_frame; ++frame) {
    { // save fluid data
      std::fstream file{"fluid." + std::to_string(frame) + ".vtk",
                        std::fstream::trunc | std::fstream::out};
      dump_fluid_vtk("fluid frame " + std::to_string(frame), f, file);
    }

    if (frame == max_frame)
      break;

    std::cout << "frame " << (frame + 1) << " scheme.execute() ..."
              << std::endl;
    for (size_t i = 0; i < 0.01f / time_step; ++i)
      scheme.execute();
    std::cout << "... done" << std::endl;
  }
}

// dump_boundary_vtk(...) {{{

template <typename Group>
void dump_boundary_vtk(std::string description, Group &group, std::ostream &s) {
  constexpr prtcl::tag::kind::varying varying;
  constexpr prtcl::tag::type::vector vector;
  constexpr prtcl::tag::type::scalar scalar;

  s << "# vtk DataFile Version 2.0\n";
  s << description << "\n";
  s << "ASCII\n";
  s << "DATASET POLYDATA\n";

  auto s_flags = s.flags();

  auto &x = group.get(varying, vector)["position"];
  auto &V = group.get(varying, scalar)["volume"];

  auto format_array = [](auto const &value) {
    std::ostringstream s;
    s << value[0];
    for (Eigen::Index i = 1; i < value.size(); ++i)
      s << " " << value[i];
    return s.str();
  };

  s << "POINTS " << group.size() << " float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(x[i]) << "\n";

  s << "POINT_DATA " << group.size() << "\n";

  s << "SCALARS volume float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << V[i] << "\n";

  s.flags(s_flags);
}

// }}}

// dump_fluid_vtk(...) {{{

template <typename Group>
void dump_fluid_vtk(std::string description, Group &group, std::ostream &s) {
  constexpr prtcl::tag::kind::varying varying;
  constexpr prtcl::tag::type::vector vector;
  constexpr prtcl::tag::type::scalar scalar;

  s << "# vtk DataFile Version 2.0\n";
  s << description << "\n";
  s << "ASCII\n";
  s << "DATASET POLYDATA\n";

  auto s_flags = s.flags();

  auto &x = group.get(varying, vector)["position"];
  auto &v = group.get(varying, vector)["velocity"];
  auto &a = group.get(varying, vector)["acceleration"];

  auto &rho = group.get(varying, scalar)["density"];
  auto &p = group.get(varying, scalar)["pressure"];

  auto format_array = [](auto const &value) {
    std::ostringstream s;
    s << value[0];
    for (Eigen::Index i = 1; i < value.size(); ++i)
      s << " " << value[i];
    return s.str();
  };

  s << "POINTS " << group.size() << " float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(x[i]) << "\n";

  s << "POINT_DATA " << group.size() << "\n";

  s << "VECTORS velocity float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(v[i]) << "\n";

  s << "VECTORS acceleration float\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << format_array(a[i]) << "\n";

  s << "SCALARS density float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << rho[i] << "\n";

  s << "SCALARS pressure float 1\n";
  s << "LOOKUP_TABLE default\n";
  for (size_t i = 0; i < group.size(); ++i)
    s << std::fixed << p[i] << "\n";

  s.flags(s_flags);
}

// }}}
