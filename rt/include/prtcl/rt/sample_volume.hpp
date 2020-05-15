#pragma once

#include <prtcl/core/identity.hpp>
#include <prtcl/core/remove_cvref.hpp>

#include <prtcl/rt/geometry/axis_aligned_box.hpp>
#include <prtcl/rt/geometry/triangle_mesh.hpp>
#include <prtcl/rt/integral_grid.hpp>

#include <array>
#include <random>
#include <type_traits>
#include <vector>

#include <cmath>

#include <boost/container/flat_set.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

#include <iostream>

namespace prtcl::rt {

struct sample_volume_parameters {
  long double maximum_sample_distance;
};

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

template <
    typename ModelPolicy_, typename OutputIt_,
    typename Transform_ = core::identity_fn>
void sample_volume(
    triangle_mesh<ModelPolicy_> const &mesh_, OutputIt_ it_,
    sample_volume_parameters const &p_, Transform_ transform = {}) {
  static constexpr size_t N = ModelPolicy_::dimensionality;

  using type_policy = typename ModelPolicy_::type_policy;
  using real = typename type_policy::real;

  using math_policy = typename ModelPolicy_::math_policy;
  using o = typename math_policy::operations;

  using rvec = typename math_policy::template ndtype_t<dtype::real, 3>;

  // compute aabb of the mesh
  rvec x_lo = o::template positive_infinity<dtype::real, N>(),
       x_hi = o::template negative_infinity<dtype::real, N>();
  for (auto const &x : mesh_.vertices()) {
    for (int n = 0; n < static_cast<int>(N); ++n) {
      x_lo[n] = std::min(x_lo[n], x[n]);
      x_hi[n] = std::max(x_hi[n], x[n]);
    }
  }

  // create a grid over the meshes aabb
  rvec step;
  integral_grid<N> grid;
  for (size_t n = 0; n < N; ++n) {
    auto const d = static_cast<int>(n);

    auto const delta = x_hi - x_lo;
    grid.extents[n] =
        static_cast<size_t>(std::round(delta[d] / p_.maximum_sample_distance));
    step[d] = delta[d] / static_cast<real>(grid.extents[n]);
    std::cerr << "DEBUG: EXTENT " << n << " " << delta[d] << " "
              << p_.maximum_sample_distance << " " << grid.extents[n] << " "
              << step[d] << std::endl;
  }

  auto make_direction = []() {
    // {{{
    std::mt19937 gen{0};
    std::uniform_real_distribution<real> rdis{-1, 1};

    rvec result;
    for (int n = 0; n < static_cast<int>(N); ++n)
      result[n] = rdis(gen);

    return result;
    // }}}
  };

  std::cerr << "DEBUG: eps " << std::sqrt(std::numeric_limits<real>::epsilon())
            << std::endl;

  auto intersect = [&mesh_](
                       auto const &base_, auto const &dvec_,
                       auto const &face_) -> std::optional<real> {
    // From:
    //    Title:   Fast, Minimum Storage Ray / Triangle Intersection
    //    Authors: Tomas Möller, Ben Trumbore
    //    Year:    ???
    // {{{
    static auto const eps = std::sqrt(std::numeric_limits<real>::epsilon());

    rvec f0 = mesh_.vertices()[face_[0]], f1 = mesh_.vertices()[face_[1]],
         f2 = mesh_.vertices()[face_[2]];
    rvec e01 = f1 - f0, e02 = f2 - f0;
    rvec pvec = o::cross(dvec_, e02);
    real det = o::dot(e01, pvec);

    if (det > -eps and det < eps)
      // ray and triangle are collinear, no intersection
      return std::nullopt;

    rvec tvec = base_ - f0;
    real u = o::dot(tvec, pvec) / det;
    if (u < 0 or u > 1)
      return std::nullopt;

    rvec qvec = o::cross(tvec, e01);
    real v = o::dot(dvec_, qvec) / det;
    if (v < 0 or u + v > 1)
      return std::nullopt;

    // TODO: if refactored into function, also return u and v, they are
    //       coordinates in the triangle plane
    return o::dot(e02, qvec) / det;
    // }}}
  };

  auto d = make_direction();
  d /= o::norm(d);

  // check each point for being interior
  // TODO: implement a non-naive variant of this check
  for (auto &g_arr : grid) {
    // compute the point on the grid
    rvec g = x_lo + o::cmul(o::template from_array<dtype::real>(g_arr), step);

    // std::cerr << "DEBUG: i " << g_arr[0] << " " << g_arr[1] << " " <<
    // g_arr[2]
    //          << std::endl;
    // std::cerr << "DEBUG: r " << g[0] << " " << g[1] << " " << g[2] <<
    // std::endl;

    int counter = 0;
    for (auto const &f : mesh_.faces()) {
      // increment the counter if the ray intersects the face
      if (intersect(g, d, f).value_or(-1) > 0) {
        ++counter;
        continue;
      }
    }

    // if the counter is odd, the point g was in the interior of the mesh
    if (counter % 2 == 1) {
      *(it_++) = transform(g);
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

} // namespace prtcl::rt
