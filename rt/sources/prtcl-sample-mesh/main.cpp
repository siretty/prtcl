#include "prtcl/rt/common.hpp"
#include <prtcl/rt/default_model_policy.hpp>
#include <prtcl/rt/sample_surface.hpp>
#include <prtcl/rt/triangle_mesh.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc_, char **argv_) {
  (void)(argc_), (void)(argv_);

  namespace rt = prtcl::rt;

  using model_policy = rt::default_model_policy<3>;
  using math_policy = typename model_policy::math_policy;
  using rvec = typename math_policy::template nd_dtype_t<rt::nd_dtype::real, 3>;

  rt::triangle_mesh<model_policy> mesh;

  { // load the mesh
    auto obj_file = std::ifstream{argv_[1]};
    mesh = rt::triangle_mesh<model_policy>::load_from_obj(obj_file);
    obj_file.close();
  }

  std::vector<rvec> points;
  rt::sample_surface(
      mesh, std::back_inserter(points), rt::surface_sample_parameters{.5L});

  for (auto const &v : points) {
    std::cout << "\"" << v[0] << " " << v[1] << " " << v[2] << " \\n\" +" << '\n';
  }
}
