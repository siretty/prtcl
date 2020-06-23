#pragma once

//#include <prtcl/core/identity.hpp>
//#include <prtcl/core/remove_cvref.hpp>
//#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include "../cxx.hpp"
#include "../log.hpp"
#include "../util/integral_grid.hpp"
#include "triangle_mesh.hpp"

#include <array>
#include <random>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

#include <iostream>

namespace prtcl {

struct SampleVolumeParameters {
  double maximum_sample_distance;
};

/*
/// Sample particles in an AAB aligned to a grid with spacing of
/// p_.maximum_sample_distance centered in the box.
template <typename ModelPolicy_, size_t N_, typename OutputIt_>
void sample_volume(
    axis_aligned_box<ModelPolicy_, N_> const &aab_, OutputIt_ it_,
    sample_volume_parameters const &p_) {
  using math_policy = typename ModelPolicy_::math_policy;
  using o = typename math_policy::operations;
  using rvec = typename math_policy::template ndtype_t<dtype::real, 3>;

  auto const delta = aab_.hi() - aab_.lo();
  rvec step, offset;

  integral_grid<N_> grid;
  for (size_t n = 0; n < N_; ++n) {
    grid.extents[n] =
        static_cast<size_t>(std::floor(delta[n] / p_.maximum_sample_distance));
    step[n] = p_.maximum_sample_distance;
    offset[n] =
        ((grid.extents[n] + 1) * p_.maximum_sample_distance - delta[n]) / 2;
  }

  rvec g_vec;
  for (auto const &g_arr : grid) {
    g_vec = o::template from_array<dtype::real>(g_arr);
    *(it_++) = aab_.lo() + o::cmul(g_vec, step) + offset;
  }
}
 */

// template <typename ModelPolicy_, size_t N_, typename OutputIt_>
// void sample_volume(
//    axis_aligned_box<ModelPolicy_, N_> const &aab_, OutputIt_ it_,
//    sample_volume_parameters const &p_) {
//  using math_policy = typename ModelPolicy_::math_policy;
//  using c = typename math_policy::constants;
//  using rvec = typename math_policy::template ndtype_t<dtype::real, 3>;
//
//  auto const delta = aab_.hi() - aab_.lo();
//  rvec step;
//
//  // TODO: introduce a "fit" parameter that determines if the particles are
//  // aligned to the sides of the box, otherwise float them in regualr
//  distances
//  // in the center of the box (avoids problems with sampling fluid that
//  // "explodes")
//
//  integral_grid<N_> grid;
//  for (size_t n = 0; n < N_; ++n) {
//    grid.extents[n] =
//        static_cast<size_t>(std::floor(delta[n] /
//        p_.maximum_sample_distance));
//    step[n] = delta[n] / grid.extents[n];
//  }
//
//  rvec g_vec;
//  for (auto const &g_arr : grid) {
//    g_vec = o::template from_array<dtype::real>(g_arr);
//    *(it_++) = aab_.lo() + (g_vec.array() * step.array()).matrix();
//  }
//}

template <typename OutputIt_>
void SampleVolume(
    TriangleMesh const &mesh, OutputIt_ it_, SampleVolumeParameters const &p_) {
  static constexpr size_t N = 3;

  using Real = double;

  using RVec = TensorT<Real, N>;

  // compute aabb of the mesh
  RVec x_lo = math::positive_infinity<Real, N>(),
       x_hi = math::negative_infinity<Real, N>();
  for (auto const &x : mesh.Vertices()) {
    x_lo = math::cmin(x_lo, x);
    x_hi = math::cmax(x_hi, x);
  }

  // create a grid over the meshes aabb
  RVec step;
  IntegralGrid<N> grid;
  for (size_t n = 0; n < N; ++n) {
    auto const d = static_cast<int>(n);

    auto const delta = x_hi - x_lo;
    grid.extents[n] =
        static_cast<size_t>(std::round(delta[d] / p_.maximum_sample_distance));
    step[d] = delta[d] / static_cast<Real>(grid.extents[n]);
    log::Debug(
        "lib", "SampleVolume", "extent=", n, " ", delta[d], " ",
        p_.maximum_sample_distance, " ", grid.extents[n], " ", step[d]);
  }

  auto make_direction = []() {
    // {{{
    std::mt19937 gen{0};
    std::uniform_real_distribution<Real> rdis{-1, 1};

    RVec result;
    for (int n = 0; n < static_cast<int>(N); ++n)
      result[n] = rdis(gen);

    return result;
    // }}}
  };

  log::Debug(
      "lib", "SampleVolume",
      "eps=", std::sqrt(std::numeric_limits<Real>::epsilon()));

  auto intersect = [&mesh](
                       auto const &base_, auto const &dvec_,
                       auto const &face_) -> std::optional<Real> {
    // From:
    //    Title:   Fast, Minimum Storage Ray / Triangle Intersection
    //    Authors: Tomas Möller, Ben Trumbore
    //    Year:    ???
    // {{{
    static auto const eps = std::sqrt(std::numeric_limits<Real>::epsilon());

    RVec f0 = mesh.Vertices()[face_[0]], f1 = mesh.Vertices()[face_[1]],
         f2 = mesh.Vertices()[face_[2]];
    RVec e01 = f1 - f0, e02 = f2 - f0;
    RVec pvec = math::cross(dvec_, e02);
    Real det = math::dot(e01, pvec);

    if (det > -eps and det < eps)
      // ray and triangle are collinear, no intersection
      return std::nullopt;

    RVec tvec = base_ - f0;
    Real u = math::dot(tvec, pvec) / det;
    if (u < 0 or u > 1)
      return std::nullopt;

    RVec qvec = math::cross(tvec, e01);
    Real v = math::dot(dvec_, qvec) / det;
    if (v < 0 or u + v > 1)
      return std::nullopt;

    // TODO: if refactored into function, also return u and v, they are
    //       coordinates in the triangle plane
    return math::dot(e02, qvec) / det;
    // }}}
  };

  auto d = make_direction();
  d /= math::norm(d);

  // check each point for being interior
  // TODO: implement a non-naive variant of this check
  for (auto &g_arr : grid) {
    // compute the point on the grid
    RVec g =
        x_lo + math::cmul(
                   RVec{
                       static_cast<Real>(g_arr[0]), static_cast<Real>(g_arr[1]),
                       static_cast<Real>(g_arr[2])},
                   step);

    // std::cerr << "DEBUG: i " << g_arr[0] << " " << g_arr[1] << " " <<
    // g_arr[2]
    //          << std::endl;
    // std::cerr << "DEBUG: r " << g[0] << " " << g[1] << " " << g[2] <<
    // std::endl;

    int counter = 0;
    for (auto const &f : mesh.Faces()) {
      // increment the counter if the ray intersects the face
      if (intersect(g, d, f).value_or(-1) > 0) {
        ++counter;
        continue;
      }
    }

    // if the counter is odd, the point g was in the interior of the mesh
    if (counter % 2 == 1) {
      //*(it_++) = transform(g);
      *(it_++) = g;
    }
    // else {
    //  std::cerr << "DEBUG: g " << g[0] << ' ' << g[1] << ' ' << g[2];

    //  std::cerr << " t";
    //  for (auto const &f : mesh_.faces()) {
    //    // increment the counter if the ray intersects the face
    //    if (auto t_opt = intersect(g, d, f)) {
    //      std::cerr << ' ' << t_opt.value();
    //    }
    //  }
    //  std::cerr << std::endl;
    //}
  }
}

} // namespace prtcl
