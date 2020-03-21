#include "prtcl/rt/common.hpp"
#include <prtcl/rt/default_model_policy.hpp>
#include <prtcl/rt/geometry/triangle_mesh.hpp>
#include <prtcl/rt/sample_surface.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc_, char **argv_) {
  (void)(argc_), (void)(argv_);

  namespace rt = prtcl::rt;

  using prtcl::core::dtype;

  using model_policy = rt::default_model_policy<3>;
  using math_policy = typename model_policy::math_policy;
  using rvec = typename math_policy::template ndtype_t<dtype::real, 3>;

  rt::triangle_mesh<model_policy> mesh;

  { // load the mesh
    auto obj_file = std::ifstream{argv_[1]};
    mesh = rt::triangle_mesh<model_policy>::load_from_obj(obj_file);
    obj_file.close();
  }

  mesh.scale(1);

  std::vector<rvec> points;

  rt::sample_surface_parameters params{.05L};
  params.sample_vertices = true;
  params.sample_edges = true;
  params.sample_faces = true;

  rt::sample_surface(mesh, std::back_inserter(points), params);

  std::cout << "vertices = [" << '\n';
  for (auto const &v : mesh.vertices()) {
    std::cout << "  [ " << v[0] << ", " << v[1] << ", " << v[2] << " ],"
              << '\n';
  }
  std::cout << "]" << '\n';
  std::cout << '\n';

  std::cout << "faces = [" << '\n';
  for (auto const &f : mesh.faces()) {
    std::cout << "  [ " << f[0] << ", " << f[1] << ", " << f[2] << " ],"
              << '\n';
  }
  std::cout << "]" << '\n';
  std::cout << '\n';

  std::cout << "points = [" << '\n';
  for (auto const &v : points) {
    std::cout << "  [ " << v[0] << ", " << v[1] << ", " << v[2] << " ],"
              << '\n';
  }
  std::cout << "]" << '\n';
}
