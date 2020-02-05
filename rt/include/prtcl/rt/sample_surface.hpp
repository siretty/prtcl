#pragma once

#include <prtcl/core/remove_cvref.hpp>
#include <prtcl/rt/triangle_mesh.hpp>

#include <array>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>

namespace prtcl::rt {

struct surface_sample_parameters {
  long double maximum_sample_distance;
  bool sample_vertices = true;
  bool sample_edges = true;
  bool sample_faces = true;
};

template <typename ModelPolicy_, typename OutputIt_>
void sample_surface(
    triangle_mesh<ModelPolicy_> const &mesh_, OutputIt_ it_,
    surface_sample_parameters p_) {
  using type_policy = typename ModelPolicy_::type_policy;
  using real = typename type_policy::template dtype_t<nd_dtype::real>;

  using math_policy = typename ModelPolicy_::math_policy;
  using o = typename math_policy::operations;

  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, 3>;
  using index_type =
      typename prtcl::core::remove_cvref_t<decltype(mesh_)>::index_type;

  auto make_edge = [](index_type lhs, index_type rhs) {
    if (lhs < rhs)
      return std::array<index_type, 2>{lhs, rhs};
    else if (rhs < lhs)
      return std::array<index_type, 2>{rhs, lhs};
    else
      throw std::runtime_error{"mesh edge nodes must be different"};
  };

  // compute the set of edges
  boost::container::flat_set<std::array<index_type, 2>> edges;
  for (auto const &f : mesh_.faces()) {
    edges.insert(make_edge(f[0], f[1]));
    edges.insert(make_edge(f[1], f[2]));
    edges.insert(make_edge(f[2], f[0]));
  }

  if (p_.sample_vertices) {
    // one sample per vertex
    for (rvec const &v : mesh_.vertices())
      *(it_++) = v;
  }

  if (p_.sample_edges) {
    // multiple samples per edge
    for (auto const &e : edges) {
      // retrieve the vertices and compute the vector from one to the other
      rvec v0 = mesh_.vertices()[e[0]], v1 = mesh_.vertices()[e[1]];
      rvec v0_to_v1 = v1 - v0;

      // ensure that each sample has __at most__ the required spacing
      auto const count = static_cast<size_t>(
          std::ceil(o::norm(v0_to_v1) / p_.maximum_sample_distance));

      // generate count samples along the edge (but skip the vertices)
      for (size_t i = 1; i < count; ++i)
        *(it_++) = rvec{v0 + static_cast<real>((1.L * i) / count) * v0_to_v1};
    }
  }

  if (p_.sample_faces) {
    // multiple samples per face
    for (auto const &f : mesh_.faces()) {
      auto v0 = mesh_.vertices()[f[0]], v1 = mesh_.vertices()[f[1]],
           v2 = mesh_.vertices()[f[2]];

      auto v0_to_v1 = v1 - v0, v0_to_v2 = v2 - v0;

      auto a102 = std::acos(
          o::dot(v0_to_v1, v0_to_v2) / o::norm(v0_to_v1) / o::norm(v0_to_v2));

      std::cerr << f[0] << ' ' << f[1] << ' ' << f[2] << '\n';
    }
  }
}

} // namespace prtcl::rt
