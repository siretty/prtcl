#pragma once

#include "../cxx.hpp"
#include "../log.hpp"
#include "../math.hpp"
#include "triangle_mesh.hpp"

#include <array>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

namespace prtcl {

struct SampleSurfaceParameters {
  double maximum_sample_distance;
  bool sample_vertices = true;
  bool sample_edges = true;
  bool sample_faces = true;
};

template <typename OutputIt_>
void SampleSurface(
    TriangleMesh const &mesh, OutputIt_ it_,
    SampleSurfaceParameters const &p_) {
  using Real = double;

  using RVec = TensorT<Real, 3>;
  using RMat2 = TensorT<Real, 2, 2>;
  using RVec2 = TensorT<Real, 2>;

  using index_type = cxx::remove_cvref_t<decltype(mesh)>::index_type;

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

  auto from_face = [&mesh](auto const &face_) {
    // {{{
    auto v0 = mesh.Vertices()[face_[0]], v1 = mesh.Vertices()[face_[1]],
         v2 = mesh.Vertices()[face_[2]];

    // std::cerr << "0 = [" << v0[0] << " " << v0[1] << " " << v0[2] << "]"
    //          << "\n";
    // std::cerr << "1 = [" << v1[0] << " " << v1[1] << " " << v1[2] << "]"
    //          << "\n";
    // std::cerr << "2 = [" << v2[0] << " " << v2[1] << " " << v2[2] << "]"
    //          << "\n";

    auto v0_to_v1 = v1 - v0, v0_to_v2 = v2 - v0, v1_to_v2 = v2 - v1;

    auto angle_between = [](auto const &a, auto const &b) {
      return std::acos(math::dot(a, b) / math::norm(a) / math::norm(b));
    };

    auto const a0 = angle_between(v0_to_v1, v0_to_v2);
    auto const a1 = angle_between(-v0_to_v1, v1_to_v2);
    auto const a2 = angle_between(-v1_to_v2, -v0_to_v2);
    // auto a0 = std::acos(
    //    math::dot(v0_to_v1, v0_to_v2) / math::norm(v0_to_v1) /
    //    math::norm(v0_to_v2));
    // auto a1 = std::acos(
    //    math::dot(-v0_to_v1, v1_to_v2) / math::norm(v0_to_v1) /
    //    math::norm(v1_to_v2));
    // auto a2 = std::acos(
    //    math::dot(-v1_to_v2, -v0_to_v2) / math::norm(v1_to_v2) /
    //    math::norm(v0_to_v2));

    if (a0 >= a1 and a0 >= a2) {
      return std::make_tuple(true, v0, v1, v2);
    } else if (a1 >= a0 and a1 >= a2) {
      return std::make_tuple(true, v1, v2, v0);
    } else if (a2 >= a0 and a2 >= a1) {
      return std::make_tuple(true, v2, v0, v1);
    } else {
      log::Warning(
          "lib", "SampleSurface", "encountered face with invalid angles");
      return std::make_tuple(false, v0, v1, v2);
    }
    // }}}
  };

  auto project = [](auto from_, auto onto_) -> RVec {
    return math::dot(from_, onto_) / math::norm_squared(onto_) * (onto_);
  };

  auto intersect = [](auto ab_, auto ad_, auto bb_, auto bd_) -> RVec2 {
    // {{{
    RMat2 A;
    A(0, 0) = math::dot(ad_, ad_), A(0, 1) = -math::dot(ad_, bd_);
    A(1, 1) = math::dot(bd_, bd_), A(1, 0) = A(0, 1);

    RVec2 b;
    b(0) = -math::dot(ad_, ab_ - bb_);
    b(1) = math::dot(bd_, ab_ - bb_);

    return math::solve_sd(A, b);
    // }}}
  };

  auto ticks_between = [&p_](RVec a_, RVec b_) {
    // {{{
    auto count = static_cast<size_t>(
        std::ceil(math::norm(a_ - b_) / p_.maximum_sample_distance));
    return boost::irange<size_t>(static_cast<size_t>(1), count) |
           boost::adaptors::transformed([count, a_, b_](size_t i_) -> RVec {
             return a_ + static_cast<Real>((1.L * i_) / count) * (b_ - a_);
           });
    // }}}
  };

  // compute the set of edges
  boost::container::flat_set<std::array<index_type, 2>> edges;
  for (auto const &f : mesh.Faces()) {
    edges.insert(make_edge(f[0], f[1]));
    edges.insert(make_edge(f[1], f[2]));
    edges.insert(make_edge(f[2], f[0]));
  }

  if (p_.sample_vertices) {
    log::Debug("lib", "SampleSurface", "sampling vertices ...");

    // one sample per vertex
    for (RVec const &v : mesh.Vertices())
      *(it_++) = v;

    log::Debug("lib", "SampleSurface", "sampling vertices ... done");
  }

  if (p_.sample_edges) {
    log::Debug("lib", "SampleSurface", "sampling edges ...");

    // multiple samples per edge
    for (auto const &e : edges) {
      // retrieve the vertices and compute the vector from one to the other
      RVec v0 = mesh.Vertices()[e[0]], v1 = mesh.Vertices()[e[1]];

      // generate count samples along the edge (but skip the vertices)
      for (auto t : ticks_between(v0, v1))
        *(it_++) = t;
    }

    log::Debug("lib", "SampleSurface", "sampling edges ... done");
  }

  if (p_.sample_faces) {
    log::Debug("lib", "SampleSurface", "sampling faces ...");

    // multiple samples per face
    for (auto const &f : mesh.Faces()) {
      // m is the point with the largest angle
      auto [ok, m, a, b] = from_face(f);

      // skip invalid faces
      if (not ok)
        continue;

      // h is the base of the height above m
      RVec h = a + project(m - a, b - a);

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

    log::Debug("lib", "SampleSurface", "sampling faces ... done");
  }
}

} // namespace prtcl
