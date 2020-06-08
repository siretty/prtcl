#pragma once

#include "common.hpp"

#include <prtcl/core/constants.hpp>
#include <prtcl/core/log/logger.hpp>

#include <cmath>

namespace prtcl::rt {

template <typename ModelPolicy_> class pinhole_camera {
public:
  using model_policy = ModelPolicy_;

private:
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using data_policy = typename ModelPolicy_::data_policy;

  using o = typename math_policy::operations;

  static constexpr auto N = model_policy::dimensionality;
  static_assert(N == 3, "");

  using real = typename type_policy::real;
  using rvec = typename math_policy::template ndtype_t<dtype::real, N>;

public:
  struct {
    rvec origin;
    rvec principal;
    rvec up;
    real focal_length;
  } camera;

  struct {
    size_t width;
    size_t height;
  } sensor;

public:
  template <typename PerRay> void cast(PerRay per_ray) const {
    namespace log = prtcl::core::log;

    // vertical, principal and horizontal directions of the camera
    rvec const v = o::normalized(camera.up);
    rvec const p = o::normalized(camera.principal);
    rvec const h = o::normalized(o::cross(v, p));

    // width of a single pixel
    real const pixel_size = static_cast<real>(1) / sensor.width;

    // origin of the sensor in the 2d image plane
    real const sensor_origin_x = -static_cast<real>(sensor.width - 1) / 2;
    real const sensor_origin_y = -static_cast<real>(sensor.height - 1) / 2;

    auto const O = camera.origin;
    auto const f = camera.focal_length;

    // log::debug(
    //    "camera", "cast", "v=", h, " h=", v, " p=", p, " sox=",
    //    sensor_origin_x, " soy=", sensor_origin_y, " O=", O, " f=", f);

    auto pixel_point = [=](auto ix, auto iy) -> rvec {
      real const pixel_x = (sensor_origin_x + ix) * pixel_size;
      real const pixel_y = (sensor_origin_y + iy) * pixel_size;

      return O + pixel_x * h + pixel_y * v - f * p;
    };

    for (size_t ix = 0; ix < sensor.width; ++ix) {
      for (size_t iy = 0; iy < sensor.height; ++iy) {
        per_ray(ix, iy, O, o::normalized(O - pixel_point(ix, iy)));
      }
    }
  }
};

} // namespace prtcl::rt
