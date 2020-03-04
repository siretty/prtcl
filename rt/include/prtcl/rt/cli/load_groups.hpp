#pragma once

#include "command_line_interface.hpp"

#include "../basic_source.hpp"
#include "../initialize_particles.hpp"
#include "../log/logger.hpp"
#include "../sample_surface.hpp"
#include "../sample_volume.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace prtcl::rt {

struct bad_sampling_method : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct bad_file_format : std::runtime_error {
  using std::runtime_error::runtime_error;
};

template <typename Model_, typename SourceIt_>
void load_model_groups_from_cli(
    command_line_interface &cli_, Model_ &model_, SourceIt_ source_it) {
  using model_policy = typename Model_::model_policy;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  using source_type = basic_source<model_policy>;

  using c = typename math_policy::constants;

  static constexpr size_t N = model_policy::dimensionality;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template nd_dtype_t<nd_dtype::real, N>;

  using triangle_mesh_type = triangle_mesh<model_policy>;

  auto get_vector = [](auto tree, auto name, auto init, auto h) {
    // {{{ implementation
    rvec result = init;
    if (auto node = tree.get_child_optional(name)) {
      for (size_t n = 0; n < N; ++n)
        result[n] = node->template get<real>(std::to_string(n), 1);
      if (node->get("adaptive", true))
        result *= h;
    }
    return result;
    // }}}
  };

  auto handle_model_parameters = [&get_vector, &model_](auto tree) {
    // {{{ implementation
    model_.template add_global<nd_dtype::real>("smoothing_scale")[0] =
        tree.get("smoothing_scale", static_cast<real>(0.025));

    model_.template add_global<nd_dtype::real>("time_step")[0] =
        tree.get("time_step", static_cast<real>(0.00001));
    model_.template add_global<nd_dtype::real>("maximum_time_step")[0] =
        tree.get(
            "maximum_time_step",
            30 * model_.template add_global<nd_dtype::real>("time_step")[0]);

    model_.template add_global<nd_dtype::real>("maximum_cfl")[0] =
        tree.get("maximum_cfl", static_cast<real>(0.7));

    model_.template add_global<nd_dtype::real>("iisph_relaxation")[0] =
        tree.get("iisph_relaxation", static_cast<real>(0.5));

    auto const h =
        model_.template get_global<nd_dtype::real>("smoothing_scale")[0];

    rvec default_gravity = c::template zeros<nd_dtype::real, N>();
    default_gravity[N > 1 ? 1 : 0] = static_cast<real>(-9.81);

    model_.template add_global<nd_dtype::real, N>("gravity")[0] =
        get_vector(tree, "gravity", default_gravity, h);
    // }}}
  };

  auto handle_fluid_parameters = [&model_](auto tree, auto &group) {
    // {{{ implementation
    group.template add_uniform<nd_dtype::real>("rest_density")[0] =
        tree.get("rest_density", static_cast<real>(1000));

    group.template add_uniform<nd_dtype::real>("compressibility")[0] =
        tree.get("compressibility", static_cast<real>(10'000'000));

    group.template add_uniform<nd_dtype::real>("viscosity")[0] =
        tree.get("viscosity", static_cast<real>(0.01));

    group.template add_uniform<nd_dtype::real>("surface_tension")[0] =
        tree.get("surface_tension", static_cast<real>(1));
    // }}}
  };

  auto handle_boundary_parameters = [&model_](auto tree, auto &group) {
    // {{{ implementation
    group.template add_uniform<nd_dtype::real>("adhesion")[0] =
        tree.get("adhesion", static_cast<real>(0));
    group.template add_uniform<nd_dtype::real>("viscosity")[0] =
        tree.get("viscosity", static_cast<real>(0.1));
    // }}}
  };

  auto handle_sample = [&get_vector, &model_](auto tree, auto it) {
    // {{{ implementation
    // get the smoothing scale from the model
    auto const h =
        model_.template get_global<nd_dtype::real>("smoothing_scale")[0];

    for (auto &[type, tree] : tree) {
      std::cerr << "  sample " << type << std::endl;

      auto what = tree.template get<std::string>("what");
      if (what == "triangle_mesh") {
        auto file_type = tree.template get<std::string>("file_type");
        auto file_path = tree.template get<std::string>("file_path");

        // scaling factors applied to the mesh before sampling
        rvec scaling = get_vector(
            tree, "scaling", c::template ones<nd_dtype::real, N>(), h);

        // translation applied to the mesh before sampling
        rvec translation = get_vector(
            tree, "translation", c::template zeros<nd_dtype::real, N>(), h);

        // open file_path for reading
        std::ifstream file{file_path, std::ios::in};

        // load the triangle mesh from the file
        triangle_mesh_type mesh;
        if (file_type == "obj")
          mesh = triangle_mesh_type::load_from_obj(file);
        else
          throw bad_file_format{file_type.c_str()};

        // apply the scaling
        mesh.scale(scaling);
        mesh.translate(translation);

        // close the file again
        file.close();

        if (type == "surface") {
          // {{{ sample.surface
          // sample the loaded triangle mesh
          sample_surface(mesh, it, {h});

          // sample the loaded triangle mesh
          // std::vector<rvec> samples;
          // sample_surface(mesh, std::back_inserter(samples), {h});

          // std::cerr << "no. surface samples = " << samples.size() <<
          // std::endl;

          //// save the old size of the group and make room for the samples
          // size_t const old_size = group.size();
          // group.resize(old_size + samples.size());

          //// get the position field from the group (_after_ the resize)
          // auto x = group.template get_varying<nd_dtype::real, N>("position");

          //// store the sampled positions in the group
          // for (size_t i = 0; i < samples.size(); ++i)
          //  x[old_size + i] = samples[i];
          // }}}
        } else if (type == "volume") {
          // {{{ sample.volume
          // auto what = tree.template get<std::string>("what");
          // if (what == "triangle_mesh") {
          //  auto file_type = tree.template get<std::string>("file_type");
          //  auto file_path = tree.template get<std::string>("file_path");

          //  // scaling factors applied to the mesh before sampling
          //  rvec scaling = c::template ones<nd_dtype::real, N>();
          //  if (auto node = tree.get_child_optional("scaling")) {
          //    for (size_t n = 0; n < N; ++n)
          //      scaling[n] *= node->template get<real>(std::to_string(n), 1);
          //    if (node->get("adaptive", true))
          //      scaling *= h;
          //  }

          //  // translation applied to the mesh before sampling
          //  rvec translation = c::template zeros<nd_dtype::real, N>();
          //  if (auto node = tree.get_child_optional("translation")) {
          //    for (size_t n = 0; n < N; ++n)
          //      translation[n] += node->template get<real>(std::to_string(n),
          //      0);
          //    if (node->get("adaptive", true))
          //      translation *= h;
          //  }

          //  // open file_path for reading
          //  std::ifstream file{file_path, std::ios::in};

          //  // load the triangle mesh from the file
          //  triangle_mesh_type mesh;
          //  if (file_type == "obj")
          //    mesh = triangle_mesh_type::load_from_obj(file);
          //  else
          //    throw bad_file_format{file_type.c_str()};

          //  // apply the scaling
          //  mesh.scale(scaling);
          //  mesh.translate(translation);

          //  // close the file again
          //  file.close();

          // sample the loaded triangle mesh
          sample_volume(mesh, it, {h});

          // sample the loaded triangle mesh
          // std::vector<rvec> samples;
          // sample_volume(mesh, std::back_inserter(samples), {h});

          // std::cerr << "no. volume samples = " << samples.size() <<
          // std::endl;

          //// save the old size of the group and make room for the samples
          // size_t const old_size = group.size();
          // group.resize(old_size + samples.size());

          //// get the position field from the group (_after_ the resize)
          // auto x = group.template get_varying<nd_dtype::real, N>("position");

          //// store the sampled positions in the group
          // for (size_t i = 0; i < samples.size(); ++i)
          //  x[old_size + i] = samples[i];
        }
        // }}}
      } else
        throw bad_sampling_method{type.c_str()};
    }
    // }}}
  };

  auto handle_source = [&source_it, &handle_sample, &get_vector,
                        &model_](auto tree, auto &group) {
    // {{{ implementation
    // get the smoothing scale from the model
    auto const h =
        model_.template get_global<nd_dtype::real>("smoothing_scale")[0];

    rvec center =
        get_vector(tree, "center", c::template zeros<nd_dtype::real, N>(), h);
    rvec velocity =
        get_vector(tree, "velocity", c::template ones<nd_dtype::real, N>(), h);
    real radius = tree.get("radius", 3 * h);
    ssize_t remaining = tree.get("count", ssize_t{10000});

    (source_it++) =
        source_type{model_, group, center, velocity, radius, remaining};
    // }}}
  };

  auto append_samples = [](auto &group, auto &samples) {
    // {{{ implementation
    // save the old size of the group and make room for the samples
    size_t const old_size = group.size();
    group.resize(old_size + samples.size());

    // get the position field from the group (_after_ the resize)
    auto x = group.template get_varying<nd_dtype::real, N>("position");

    // store the sampled positions in the group
    for (size_t i = 0; i < samples.size(); ++i)
      x[old_size + i] = samples[i];
    // }}}
  };

  auto handle_model_group = [&handle_sample, &handle_source, &append_samples,
                             &handle_fluid_parameters,
                             &handle_boundary_parameters,
                             &model_](auto group_name, auto group_tree) {
    // {{{ implementation
    // fetch the group type with a default
    auto group_type = group_tree.template get<std::string>("type", "particle");

    log::debug(
        "app", "setup_model", "group ", group_name, " with type ", group_type);

    // add the group to the model
    auto &group = model_.add_group(group_name, group_type);

    // add the group tags
    for (auto [it, last] = group_tree.equal_range("tag"); it != last; ++it)
      group.add_tag(it->second.template get_value<std::string>());

    // initialize the group fields (without initializing any particles)
    initialize_particles(model_, group, boost::irange(0, 0));

    { // load parameters for group
      // TODO: HACK: implement proper defaults
      boost::property_tree::ptree params;
      if (auto params_opt = group_tree.get_child_optional("parameters"))
        params = params_opt.value();
      if (group_type == "fluid")
        handle_fluid_parameters(params, group);
      if (group_type == "boundary")
        handle_boundary_parameters(params, group);
    }

    // add the position field to the group
    group.template add_varying<nd_dtype::real, N>("position");

    // temporary storage for samples
    std::vector<rvec> samples;

    // sample the initial configuration
    for (auto [it, last] = group_tree.equal_range("sample"); it != last; ++it)
      handle_sample(it->second, std::back_inserter(samples));

    std::cerr << "no. surface samples = " << samples.size() << std::endl;

    // append the sampled position to the group
    append_samples(group, samples);

    for (auto [it, last] = group_tree.equal_range("source"); it != last; ++it)
      handle_source(it->second, group);
    // }}}
  };

  auto &nv = cli_.name_value();
  for (auto [it, last] = nv.equal_range("model"); it != last; ++it) {
    auto &model = it->second;

    { // load parameters (_must_ be done before loading the groups)
      // TODO: HACK: implement proper defaults
      boost::property_tree::ptree params;
      if (auto params_opt = model.get_child_optional("parameters"))
        params = params_opt.value();
      handle_model_parameters(params);
    }

    for (auto [it, last] = model.equal_range("group"); it != last; ++it) {
      auto &groups = it->second;
      for (auto &[group_name, group_tree] : groups) {
        handle_model_group(group_name, group_tree);
      }
    }
  }
}

} // namespace prtcl::rt
