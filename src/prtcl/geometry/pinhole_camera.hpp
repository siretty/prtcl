#pragma once

#include "../math.hpp"

#include <cmath>

namespace prtcl {

class PinholeCamera {
private:
  static constexpr auto N = 3;
  static_assert(N == 3, "");

  using Real = double;
  using RVec = TensorT<Real, N>;

public:
  struct {
    RVec origin;
    RVec principal;
    RVec up;
    Real focal_length;
  } camera;

  struct {
    size_t width;
    size_t height;
  } sensor;

public:
  template <typename PerRay>
  void Cast(PerRay per_ray) const {
    // vertical, principal and horizontal directions of the camera
    RVec const v = math::normalized(camera.up);
    RVec const p = math::normalized(camera.principal);
    RVec const h = math::normalized(math::cross(v, p));

    // width of a single pixel
    Real const pixel_size = static_cast<Real>(1) / static_cast<Real>(sensor.width);

    // origin of the sensor in the 2d image plane
    Real const sensor_origin_x = -static_cast<Real>(sensor.width - 1) / 2;
    Real const sensor_origin_y = -static_cast<Real>(sensor.height - 1) / 2;

    auto const O = camera.origin;
    auto const f = camera.focal_length;

    // log::debug(
    //    "camera", "cast", "v=", h, " h=", v, " p=", p, " sox=",
    //    sensor_origin_x, " soy=", sensor_origin_y, " O=", O, " f=", f);

    auto pixel_point = [=](auto ix, auto iy) -> RVec {
      Real const pixel_x = (sensor_origin_x + ix) * pixel_size;
      Real const pixel_y = (sensor_origin_y + iy) * pixel_size;

      return O + pixel_x * h + pixel_y * v - f * p;
    };

    for (size_t ix = 0; ix < sensor.width; ++ix) {
      for (size_t iy = 0; iy < sensor.height; ++iy) {
        per_ray(ix, iy, O, math::normalized(O - pixel_point(ix, iy)));
      }
    }
  }
};

} // namespace prtcl
