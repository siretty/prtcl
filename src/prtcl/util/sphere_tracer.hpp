#ifndef PRTCL_SPHERE_TRACER_HPP_BBB18FE15818402A9C136B756757F8F4
#define PRTCL_SPHERE_TRACER_HPP_BBB18FE15818402A9C136B756757F8F4

#include "../data/model.hpp"
#include "../math.hpp"
#include "../math/kernel/cubic_spline_kernel.hpp"

#include <utility>
#include <vector>

#include <cmath>

#include <ungrd/compact_grid.hpp>
#include <ungrd/cxx/lexicographic_indexing.hpp>

namespace prtcl {

class Image {
public:
  size_t width() const { return indexing_.extent(0); }

  size_t height() const { return indexing_.extent(1); }

public:
  auto &operator()(size_t ix, size_t iy) {
    auto const index = indexing_.encode({ix, iy});
    return intensity_[index];
  }

  auto const &operator()(size_t ix, size_t iy) const {
    auto const index = indexing_.encode({ix, iy});
    return intensity_[index];
  }

public:
  Image(size_t _width, size_t _height) : indexing_{{_width, _height}} {
    intensity_.resize(indexing_.size());
  }

private:
  ungrd::lexicographic_indexing<2> indexing_;
  std::vector<double> intensity_;
};

template <typename TCamera>
class SphereTracer {
public:
  using Camera = TCamera;
  using Real = typename Camera::Real;
  using RVec = typename Camera::RVec;

private:
  using Grid = ungrd::s32_e32_compact_grid<3>;
  using CPos = typename Grid::space_policy::position;
  using Entry = typename Grid::entry_policy ::entry;

  struct GroupData {
    VaryingFieldWrap<double, 3> x;
    Grid grid;
  };

  struct RayData {
    size_t ix, iy;
    RVec origin, direction;
    Real parameter;

    RVec normal;
  };

public:
  Image Trace(Model const &model) const {
    std::vector<std::pair<CPos, Entry>> input;
    Real const grid_diameter =
        4 * model.GetGlobal().FieldWrap<Real>("smoothing_scale");

    RVec const cell_center_delta = [&grid_diameter] {
      RVec result;
      for (size_t dim = 0; dim < 3; ++dim)
        result[dim] = grid_diameter / 2;
      return result;
    }();

    std::vector<GroupData> groups;
    for (auto const &group : model.GetGroups()) {
      if (group.HasTag("visible")) {
        if (auto x = group.GetVarying().FieldWrap<Real, 3>("position")) {
          auto &gd = groups.emplace_back(GroupData{std::move(x), Grid{}});

          input.resize(gd.x.GetSize());
          for (size_t entry = 0; entry < gd.x.GetSize(); ++entry) {
            auto const entry_x = gd.x.Get(entry);
            for (size_t dim = 0; dim < 3; ++dim) {
              input[entry].first[dim] =
                  std::floor(entry_x[dim] / grid_diameter);
            }
            input[entry].second = static_cast<Entry>(entry);
          }

          gd.grid.update(input);
        }
      }
    }

    std::vector<RayData> rays;
    camera_.Cast([&rays](size_t ix, size_t iy, auto origin, auto direction) {
      rays.emplace_back(
          RayData{ix, iy, origin, direction, Real{0}, math::zeros<Real, 3>()});
    });

    Real const max_parameter = grid_diameter * 10'000;
    size_t const max_sdf_steps = max_sdf_steps_;

#pragma omp parallel for default(none) schedule(guided) shared(                \
    cell_center_delta, rays, grid_diameter, groups, max_parameter,             \
    max_sdf_steps)
    for (size_t ray_i = 0; ray_i < size(rays); ++ray_i) {
      auto &rd = rays[ray_i];

      size_t sdf_steps = 0;
      for (; sdf_steps < max_sdf_steps; ++sdf_steps) {
        RVec const ray_x = rd.origin + rd.parameter * rd.direction;

        Real sdf = math::most_positive<Real>();

        for (auto &gd : groups) {
          gd.grid.foreach_position([&](CPos const &cpos) {
            RVec const cell_center = [&] {
              RVec result;
              for (size_t dim = 0; dim < 3; ++dim)
                result[dim] =
                    grid_diameter * cpos[dim] + cell_center_delta[dim];
              return result;
            }();

            RVec const q = math::cabs(ray_x - cell_center) - grid_diameter;
            Real const l = math::norm(math::cmax(q, math::zeros<Real, 3>())) +
                           std::min(math::maximum_component(q), Real{0}) -
                           grid_diameter;
            sdf = std::min(sdf, l);
          });
        }

        // if sdf < grid_diameter -> do neighborhood search and compute CSG
        // intersection (max) with the sdf value
        if (sdf < grid_diameter) {
          constexpr auto W = math::cubic_spline_kernel<Real, 3>{};
          auto const L = W.lipschitz(grid_diameter / 2);

          CPos const ray_c = [&] {
            CPos cpos;
            for (size_t dim = 0; dim < 3; ++dim)
              cpos[dim] = std::floor(ray_x[dim] / grid_diameter);
            return cpos;
          }();

          Real phi = threshold_ * W.evalr(0, grid_diameter / 2, 3);
          RVec phi_grad = math::zeros<Real, 3>();

          // size_t n_sdf_count = 0;
          // Real n_sdf = math::most_positive<Real>();

          size_t L_count = 0;

          for (int cx = -1; cx <= 1; ++cx) {
            for (int cy = -1; cy <= 1; ++cy) {
              for (int cz = -1; cz <= 1; ++cz) {
                CPos const cpos{ray_c[0] + cx, ray_c[1] + cy, ray_c[2] + cz};

                for (auto &gd : groups) {
                  gd.grid.foreach_entry_at_position(
                      cpos, [&](auto const entry) {
                        auto const entry_x = gd.x.Get(entry);

                        // n_sdf = std::min(
                        //    n_sdf, math::norm(ray_x - entry_x) -
                        //    grid_diameter);
                        //++n_sdf_count;

                        auto const w =
                            W.eval(ray_x - entry_x, grid_diameter / 2);

                        if (w > 0) {
                          phi -= w;
                          phi_grad -=
                              W.evalgrad(ray_x - entry_x, grid_diameter / 2);
                          ++L_count;
                        }
                      });
                }
              }
            }
          }

          if (L_count > 0) {
            phi /= L * L_count;
            rd.normal = phi_grad / (L * L_count);

            sdf = std::max(sdf, phi);
          } else {
            sdf = std::max(sdf, grid_diameter / 4);
          }
          // else if (n_sdf_count > 0) {
          //  sdf = std::max(sdf, n_sdf);
          //}
        }

        if (sdf < static_cast<Real>(1e-6))
          break;

        rd.parameter += sdf;

        if (rd.parameter >= max_parameter)
          break;
      }

      if (sdf_steps >= max_sdf_steps)
        rd.parameter = max_parameter;
    }

    Image image{camera_.sensor.width, camera_.sensor.height};

    Real smallest_good_parameter = math::most_positive<Real>();
    Real largest_good_parameter = Real{0};
    for (auto const &ray : rays) {
      if (ray.parameter >= max_parameter)
        continue;

      smallest_good_parameter =
          std::min(ray.parameter, smallest_good_parameter);
      largest_good_parameter = std::max(ray.parameter, largest_good_parameter);
    }

    for (auto const &ray : rays) {
      if (ray.parameter == max_parameter) {
        image(ray.ix, ray.iy) = Real{0};
      } else if (ray.parameter > max_parameter) {
        image(ray.ix, ray.iy) = Real{0.3};
      } else {
        image(ray.ix, ray.iy) = -math::dot(
            math::normalized(ray.direction), math::normalized(ray.normal));

        // image(ray.ix, ray.iy) = std::clamp(
        //    (ray.parameter - smallest_good_parameter) /
        //        (largest_good_parameter - smallest_good_parameter),
        //    Real{0}, Real{1});
      }
    }

    return image;
  }

public:
  Real GetThreshold() const { return threshold_; }

  void SetThreshold(const Real value) { threshold_ = value; }

  size_t GetMaxSteps() const { return max_sdf_steps_; }

  void SetMaxSteps(const size_t value) { max_sdf_steps_ = value; }

public:
  SphereTracer(Camera camera) : camera_{camera} {}

private:
  Camera camera_;
  Real threshold_ = 0.5;
  size_t max_sdf_steps_ = 300;
};

} // namespace prtcl

#endif // PRTCL_SPHERE_TRACER_HPP_BBB18FE15818402A9C136B756757F8F4
