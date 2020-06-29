#include "module_geometry.hpp"

#include <prtcl/data/group.hpp>
#include <prtcl/data/model.hpp>
#include <prtcl/geometry/pinhole_camera.hpp>
#include <prtcl/geometry/sample_surface.hpp>
#include <prtcl/geometry/sample_volume.hpp>
#include <prtcl/geometry/triangle_mesh.hpp>
#include <prtcl/math.hpp>

#include <fstream>
#include <vector>

namespace prtcl::lua {

sol::table ModuleGeometry(sol::state_view lua) {
  auto m = lua.create_table();

  using RVec3 = TensorT<double, 3>;
  using SVec2 = TensorT<int64_t, 2>;

  {
    auto t = m.new_usertype<TriangleMesh>("triangle_mesh", sol::no_constructor);

    t.set_function("from_obj_file", [](std::string path) {
      std::ifstream file{path};
      return TriangleMesh::load_from_obj(file);
    });

    t.set_function("scale", [](TriangleMesh &mesh, RealVector const &factors) {
      mesh.Scale(RVec3{factors});
    });

    t.set_function(
        "translate", [](TriangleMesh &mesh, RealVector const &shift) {
          mesh.Translate(RVec3{shift});
        });

    t.set_function(
        "rotate",
        [](TriangleMesh &mesh, RealScalar angle, RealVector const &axis) {
          mesh.Rotate(angle, RVec3{axis});
        });

    t.set_function(
        "sample_surface", [](TriangleMesh const &mesh, Group &group) {
          Model const &model = group.GetModel();
          std::vector<RVec3> samples;
          auto const params = SampleSurfaceParameters{
              model.GetGlobal().FieldWrap<double>("smoothing_scale"), true,
              true, true};
          SampleSurface(mesh, std::back_inserter(samples), params);
          auto indices = group.CreateItems(samples.size());
          auto x = group.GetVarying().FieldWrap<double, 3>("position");
          for (size_t i = 0; i < samples.size(); ++i)
            x[indices[i]] = samples[i];
        });

    t.set_function("sample_volume", [](TriangleMesh const &mesh, Group &group) {
      Model const &model = group.GetModel();
      std::vector<RVec3> samples;
      auto const params = SampleVolumeParameters{
          model.GetGlobal().FieldWrap<double>("smoothing_scale")};
      SampleVolume(mesh, std::back_inserter(samples), params);
      auto indices = group.CreateItems(samples.size());
      auto x = group.GetVarying().FieldWrap<double, 3>("position");
      for (size_t i = 0; i < samples.size(); ++i)
        x[indices[i]] = samples[i];
    });
  }

  {
    auto t = m.new_usertype<PinholeCamera>(
        "pinhole_camera", sol::constructors<PinholeCamera()>());
    t["origin"] = sol::property(
        [](PinholeCamera const &self) -> RealVector {
          return self.camera.origin;
        },
        [](PinholeCamera &self, RealVector const &value) {
          self.camera.origin = value;
        });
    t["principal"] = sol::property(
        [](PinholeCamera const &self) -> RealVector {
          return self.camera.principal;
        },
        [](PinholeCamera &self, RealVector const &value) {
          self.camera.principal = value;
        });
    t["up"] = sol::property(
        [](PinholeCamera const &self) -> RealVector { return self.camera.up; },
        [](PinholeCamera &self, RealVector const &value) {
          self.camera.up = value;
        });
    t["focal_length"] = sol::property(
        [](PinholeCamera const &self) -> RealScalar {
          return self.camera.focal_length;
        },
        [](PinholeCamera &self, RealScalar const &value) {
          self.camera.focal_length = value;
        });
    t["sensor_width"] = sol::property(
        [](PinholeCamera const &self) -> size_t { return self.sensor.width; },
        [](PinholeCamera &self, size_t const &value) {
          self.sensor.width = value;
        });
    t["sensor_height"] = sol::property(
        [](PinholeCamera const &self) -> size_t { return self.sensor.height; },
        [](PinholeCamera &self, size_t const &value) {
          self.sensor.height = value;
        });

    t.set_function("sample", [](PinholeCamera const &camera, Group &group) {
      auto const &model = group.GetModel();
      auto const &global = model.GetGlobal();
      auto aabb_min = global.FieldWrap<double, 3>("position_aabb_min").Get();
      auto aabb_max = global.FieldWrap<double, 3>("position_aabb_max").Get();

      auto aabb_ray_intersect = [](RVec3 const &aabb_min, RVec3 const &aabb_max,
                                   RVec3 const &ray_x, RVec3 const &ray_d,
                                   double &ray_t) -> bool {
        // based on the simple intersection implementation from
        // https://tavianator.com/fast-branchless-raybounding-box-intersections/

        double tmin = math::negative_infinity<double>(),
               tmax = math::positive_infinity<double>();

        for (size_t dim = 0; dim < 3; ++dim) {
          if (ray_d[dim] != 0.0) {
            double t1 = (aabb_min[dim] - ray_x[dim]) / ray_d[dim];
            double t2 = (aabb_max[dim] - ray_x[dim]) / ray_d[dim];

            tmin = math::cmax(tmin, math::cmin(t1, t2));
            tmax = math::cmin(tmax, math::cmax(t1, t2));
          }
        }

        if (tmax >= tmin)
          ray_t = tmin;

        return tmax >= tmin;
      };

      std::vector<SVec2> samples_s;
      std::vector<RVec3> samples_x, samples_d;
      std::vector<double> samples_t;
      camera.Cast([&samples_s, &samples_x, &samples_d, &samples_t,
                   &aabb_ray_intersect, aabb_min, aabb_max](
                      size_t ix, size_t iy, auto const &x, auto const &d) {
        double t;
        if (aabb_ray_intersect(aabb_min, aabb_max, x, d, t)) {
          samples_s.emplace_back(
              static_cast<int64_t>(ix), static_cast<int64_t>(iy));
          samples_x.emplace_back(x);
          samples_d.emplace_back(d);
          samples_t.emplace_back(t);
        }
      });

      auto indices = group.CreateItems(samples_x.size());
      auto s = group.GetVarying().FieldWrap<int64_t, 2>("sensor_position");
      auto x = group.GetVarying().FieldWrap<double, 3>("position");
      auto o = group.GetVarying().FieldWrap<double, 3>("initial_position");
      auto t = group.GetVarying().FieldWrap<double>("initial_parameter");
      auto d = group.GetVarying().FieldWrap<double, 3>("direction");
      for (size_t i = 0; i < indices.size(); ++i) {
        s[indices[i]] = samples_s[i];
        x[indices[i]] = samples_x[i];
        o[indices[i]] = samples_x[i];
        t[indices[i]] = samples_t[i];
        d[indices[i]] = samples_d[i];
      }
    });
  }

  return m;
}

} // namespace prtcl::lua
