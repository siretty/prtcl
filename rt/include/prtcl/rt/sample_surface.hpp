#pragma once

#include <prtcl/core/remove_cvref.hpp>
#include <prtcl/rt/triangle_mesh.hpp>

#include <array>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

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
  using rmat2 = typename math_policy::template nd_dtype_t<nd_dtype::real, 2, 2>;
  using rvec2 = typename math_policy::template nd_dtype_t<nd_dtype::real, 2>;

  using index_type =
      typename prtcl::core::remove_cvref_t<decltype(mesh_)>::index_type;

  auto make_edge = [](index_type lhs, index_type rhs) {
    // {{{
    if (lhs < rhs)
      return std::array<index_type, 2>{lhs, rhs};
    else if (rhs < lhs)
      return std::array<index_type, 2>{rhs, lhs};
    else
      throw std::runtime_error{"mesh edge nodes must be different"};
    // }}}
  };

  auto from_face = [&mesh_](auto const &face_) {
    // {{{
    auto v0 = mesh_.vertices()[face_[0]], v1 = mesh_.vertices()[face_[1]],
         v2 = mesh_.vertices()[face_[2]];

    // std::cerr << "0 = [" << v0[0] << " " << v0[1] << " " << v0[2] << "]"
    //          << "\n";
    // std::cerr << "1 = [" << v1[0] << " " << v1[1] << " " << v1[2] << "]"
    //          << "\n";
    // std::cerr << "2 = [" << v2[0] << " " << v2[1] << " " << v2[2] << "]"
    //          << "\n";

    auto v0_to_v1 = v1 - v0, v0_to_v2 = v2 - v0, v1_to_v2 = v2 - v1;

    auto a0 = std::acos(
        o::dot(v0_to_v1, v0_to_v2) / o::norm(v0_to_v1) / o::norm(v0_to_v2));
    auto a1 = std::acos(
        o::dot(-v0_to_v1, v1_to_v2) / o::norm(v0_to_v1) / o::norm(v1_to_v2));
    auto a2 = std::acos(
        o::dot(-v1_to_v2, -v0_to_v2) / o::norm(v1_to_v2) / o::norm(v0_to_v2));

    if (a0 >= a1 and a0 >= a2) {
      return std::make_tuple(v0, v1, v2);
    } else if (a1 >= a0 and a1 >= a2) {
      return std::make_tuple(v1, v2, v0);
    } else if (a2 >= a0 and a2 >= a1) {
      return std::make_tuple(v2, v0, v1);
    } else {
      throw std::runtime_error{"invalid triangle angles"};
    }
    // }}}
  };

  auto project = [](auto from_, auto onto_) -> rvec {
    return o::dot(from_, onto_) / o::norm_squared(onto_) * (onto_);
  };

  auto intersect = [](auto ab_, auto ad_, auto bb_, auto bd_) -> rvec2 {
    // {{{
    rmat2 A;
    A(0, 0) = o::dot(ad_, ad_) / 2, A(0, 1) = -o::dot(ad_, bd_) / 2;
    A(1, 1) = o::dot(bd_, bd_) / 2, A(1, 0) = A(0, 1);

    rvec2 b;
    b(0) = -o::dot(ad_, ab_ - bb_);
    b(1) = o::dot(bd_, ab_ - bb_);

    return o::solve_sd(A, b);
    // }}}
  };

  auto ticks_between = [&p_](rvec a_, rvec b_) {
    // {{{
    auto count = static_cast<size_t>(std::ceil(
        static_cast<long double>(o::norm(a_ - b_)) /
        p_.maximum_sample_distance));
    return boost::irange<size_t>(static_cast<size_t>(1), count) |
           boost::adaptors::transformed([count, a_, b_](size_t i_) -> rvec {
             return a_ + static_cast<real>((1.L * i_) / count) * (b_ - a_);
           });
    // }}}
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

      // generate count samples along the edge (but skip the vertices)
      for (auto t : ticks_between(v0, v1))
        *(it_++) = t;
    }
  }

  if (p_.sample_faces) {
    // multiple samples per face
    for (auto const &f : mesh_.faces()) {
      // m is the point with the largest angle
      auto [m, a, b] = from_face(f);
      // h is the base of the height above m
      auto h = a + project(m - a, b - a);

      // std::cerr << "m = [" << m[0] << " " << m[1] << " " << m[2] << "]"
      //          << "\n";
      // std::cerr << "a = [" << a[0] << " " << a[1] << " " << a[2] << "]"
      //          << "\n";
      // std::cerr << "b = [" << b[0] << " " << b[1] << " " << b[2] << "]"
      //          << "\n";
      // std::cerr << "h = [" << h[0] << " " << h[1] << " " << h[2] << "]"
      //          << "\n";
      // std::cerr << "angle m: "
      //          << std::acos(
      //                 o::dot(m - a, m - b) / o::norm(m - a) / o::norm(m - b))
      //          << '\n';

      for (auto g : ticks_between(m, h)) {
        // std::cerr << "g = [" << g[0] << " " << g[1] << " " << g[2] << "]"
        //          << "\n";

        auto l = g + intersect(g, b - a, m, b - m)[0] * (b - a);
        auto r = g + intersect(g, a - b, m, a - m)[0] * (a - b);

        // std::cerr << "l[0] = " << l[0] << " l[1] = " << l[1] << '\n';
        // std::cerr << "r[0] = " << r[0] << " r[1] = " << r[1] << '\n';

        for (auto t : ticks_between(l, r))
          *(it_++) = t;
      }
    }
  }
}

} // namespace prtcl::rt
