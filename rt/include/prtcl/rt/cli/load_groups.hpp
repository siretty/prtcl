#pragma once

#include "command_line_interface.hpp"

#include "../basic_bcc_lattice_source.hpp"
#include "../basic_fcc_lattice_source.hpp"
#include "../basic_hcp_lattice_source.hpp"
#include "../basic_source.hpp"
#include "../initialize_particles.hpp"
#include "../sample_surface.hpp"
#include "../sample_volume.hpp"
#include <prtcl/rt/basic_scg_lattice_source.hpp>

#include <prtcl/core/log/logger.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
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
  namespace log = core::log;

  using model_policy = typename Model_::model_policy;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;

  using source_type = basic_hcp_lattice_source<model_policy>;

  using o = typename math_policy::operations;

  static constexpr size_t N = model_policy::dimensionality;

  using real = typename type_policy::real;
  using rvec = typename math_policy::template ndtype_t<dtype::real, N>;
  using rmat = typename math_policy::template ndtype_t<dtype::real, N, N>;

  using triangle_mesh_type = triangle_mesh<model_policy>;

  std::mt19937 gen{0};
  std::uniform_real_distribution<real> dis;
  using dis_param = typename decltype(dis)::param_type;

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
    model_.template add_global<dtype::real>("smoothing_scale")[0] =
        tree.get("smoothing_scale", static_cast<real>(0.025));

    model_.template add_global<dtype::real>("time_step")[0] =
        tree.get("time_step", static_cast<real>(0.00001));
    model_.template add_global<dtype::real>("maximum_time_step")[0] = tree.get(
        "maximum_time_step",
        30 * model_.template add_global<dtype::real>("time_step")[0]);

    model_.template add_global<dtype::real>("maximum_cfl")[0] =
        tree.get("maximum_cfl", static_cast<real>(0.7));

    model_.template add_global<dtype::real>("iisph_relaxation")[0] =
        tree.get("iisph_relaxation", static_cast<real>(0.5));

    auto const h =
        model_.template get_global<dtype::real>("smoothing_scale")[0];

    rvec default_gravity = o::template zeros<dtype::real, N>();
    default_gravity[N > 1 ? 1 : 0] = static_cast<real>(-9.81);

    model_.template add_global<dtype::real, N>("gravity")[0] =
        get_vector(tree, "gravity", default_gravity, h);
    // }}}
  };

  auto handle_fluid_parameters = [&model_](auto tree, auto &group) {
    // {{{ implementation
    group.template add_uniform<dtype::real>("rest_density")[0] =
        tree.get("rest_density", static_cast<real>(1000));

    group.template add_uniform<dtype::real>("compressibility")[0] =
        tree.get("compressibility", static_cast<real>(10'000'000));

    group.template add_uniform<dtype::real>("viscosity")[0] =
        tree.get("viscosity", static_cast<real>(0.01));

    group.template add_uniform<dtype::real>("surface_tension")[0] =
        tree.get("surface_tension", static_cast<real>(1));

    if (auto opt = tree.template get_optional<real>("pt16_viscosity"))
      group.template add_uniform<dtype::real>("pt16_viscosity")[0] =
          opt.value();

    if (auto opt = tree.template get_optional<real>("pt16_vorticity_error"))
      group.template add_uniform<dtype::real>(
          "pt16_vorticity_diffusion_maximum_error")[0] = opt.value();

    if (auto opt = tree.template get_optional<int>("pt16_vorticity_iters"))
      group.template add_uniform<dtype::integer>(
          "pt16_vorticity_diffusion_maximum_iterations")[0] = opt.value();

    if (auto opt = tree.template get_optional<real>("pt16_velocity_error"))
      group.template add_uniform<dtype::real>(
          "pt16_velocity_reconstruction_maximum_error")[0] = opt.value();

    if (auto opt = tree.template get_optional<int>("pt16_velocity_iters"))
      group.template add_uniform<dtype::integer>(
          "pt16_velocity_reconstruction_maximum_iterations")[0] = opt.value();

    if (auto opt = tree.template get_optional<real>("wkbb18_maximum_error"))
      group.template add_uniform<dtype::real>("wkbb18_maximum_error")[0] =
          opt.value();

    if (auto opt = tree.template get_optional<int>("wkbb18_maximum_iterations"))
      group.template add_uniform<dtype::integer>(
          "wkbb18_maximum_iterations")[0] = opt.value();
    // }}}
  };

  auto handle_boundary_parameters = [&model_](auto tree, auto &group) {
    // {{{ implementation
    group.template add_uniform<dtype::real>("adhesion")[0] =
        tree.get("adhesion", static_cast<real>(0));
    group.template add_uniform<dtype::real>("viscosity")[0] =
        tree.get("viscosity", static_cast<real>(0.1));
    // }}}
  };

  auto handle_sample = [&get_vector, &model_](auto tree, auto it) {
    // {{{ implementation
    // get the smoothing scale from the model
    auto const h =
        model_.template get_global<dtype::real>("smoothing_scale")[0];

    for (auto &[type, tree] : tree) {
      std::cerr << "  sample " << type << std::endl;

      auto what = tree.template get<std::string>("what");
      if (what == "triangle_mesh") {
        auto file_type = tree.template get<std::string>("file_type");
        auto file_path = tree.template get<std::string>("file_path");

        // scaling factors applied to the mesh before sampling
        rvec scaling =
            get_vector(tree, "scaling", o::template ones<dtype::real, N>(), h);

        // translation applied to the mesh before sampling
        rvec translation = get_vector(
            tree, "translation", o::template zeros<dtype::real, N>(), h);

        // rotation applied to the particles after sampling
        rmat rotation = o::template identity<dtype::real, N, N>();

        if constexpr (N == 3) {
          // 3D: rotation from axis and angle
          rvec const axis = o::normalized(get_vector(
              tree, "rotation_axis", o::template zeros<dtype::real, N>(), 1));
          real const angle = tree.get("rotation_angle", real{0});

          rotation *=
              std::cos(angle) * o::template identity<dtype::real, N, N>() +
              std::sin(angle) * o::cross_product_matrix_from_vector(axis) +
              (1 - std::cos(angle)) * o::outer_product(axis, axis);
        }

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
          sample_surface(
              mesh, it, {h * tree.template get("sample_factor", real{1})});
          // }}}
        } else if (type == "volume") {
          // {{{ sample.volume
          // sample the loaded triangle mesh
          sample_volume(
              mesh, it, {h * tree.template get("sample_factor", real{1})},
              [&rotation](auto x) -> rvec { return rotation * x; });
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
        model_.template get_global<dtype::real>("smoothing_scale")[0];

    rvec center =
        get_vector(tree, "center", o::template zeros<dtype::real, N>(), h);
    rvec velocity =
        get_vector(tree, "velocity", o::template ones<dtype::real, N>(), h);
    real radius = h * tree.get("radius", real{3});
    ssize_t remaining = tree.get("count", ssize_t{10000});

    (source_it++) =
        source_type{model_, group, center, velocity, radius, remaining};
    // }}}
  };

  auto append_samples = [&model_, &dis, &gen](auto &group, auto &samples) {
    // {{{ implementation
    // get the smoothing scale from the model
    auto const h =
        model_.template get_global<dtype::real>("smoothing_scale")[0];

    // save the old size of the group and make room for the samples
    size_t const old_size = group.size();
    group.resize(old_size + samples.size());

    // get the position field from the group (_after_ the resize)
    auto x = group.template get_varying<dtype::real, N>("position");

    // store the sampled positions in the group
    for (size_t i = 0; i < samples.size(); ++i) {
      x[old_size + i] = samples[i];
      // perturb the sampled point slightly
      for (size_t d = 0; d < N; ++d)
        x[old_size + i][d] += dis(gen, dis_param{-h / 20, h / 20});
    }
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
    group.template add_varying<dtype::real, N>("position");

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
} // namespace prtcl::rt

} // namespace prtcl::rt
