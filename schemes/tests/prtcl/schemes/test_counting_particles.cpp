#include <catch2/catch.hpp>

#include <prtcl/rt/basic_model.hpp>
#include <prtcl/rt/basic_model_policy.hpp>
#include <prtcl/rt/basic_type_policy.hpp>
#include <prtcl/rt/eigen_math_policy.hpp>
#include <prtcl/rt/grouped_uniform_grid.hpp>
#include <prtcl/rt/neighborhood.hpp>
#include <prtcl/rt/vector_data_policy.hpp>

#include <prtcl/schemes/test.hpp>

TEST_CASE("prtcl/schemes/test_counting_particles", "[prtcl][schemes]") {
  using prtcl::rt::nd_dtype;

  using model_policy = prtcl::rt::basic_model_policy<
      prtcl::rt::fib_type_policy, prtcl::rt::eigen_math_policy,
      prtcl::rt::vector_data_policy, 1>;

  using math_policy = typename model_policy::math_policy;

  using model_type = prtcl::rt::basic_model<model_policy>;
  using scheme_type = prtcl::schemes::test<model_policy>;

  model_type model;
  model.add_group("n1", "neighbors").resize(123);
  model.add_group("n2", "neighbors").resize(456);
  model.add_group("p1", "particles").resize(789);
  model.add_group("p2", "particles").resize(123);
  model.add_group("o1", "neither").resize(456);
  model.add_group("o2", "neither").resize(789);

  // ensure all 'particles' see all 'neighbors' by placing them all at 0
  {
    auto n1x = model.get_group("n1").add_varying<nd_dtype::real, 1>("position");
    for (size_t i = 0; i < n1x.size(); ++i)
      n1x[i] = math_policy::constants::zeros<nd_dtype::real, 1>();

    auto n2x = model.get_group("n2").add_varying<nd_dtype::real, 1>("position");
    for (size_t i = 0; i < n2x.size(); ++i)
      n2x[i] = math_policy::constants::zeros<nd_dtype::real, 1>();

    auto p1x = model.get_group("p1").add_varying<nd_dtype::real, 1>("position");
    for (size_t i = 0; i < p1x.size(); ++i)
      p1x[i] = math_policy::constants::zeros<nd_dtype::real, 1>();

    auto p2x = model.get_group("p2").add_varying<nd_dtype::real, 1>("position");
    for (size_t i = 0; i < p2x.size(); ++i)
      p2x[i] = math_policy::constants::zeros<nd_dtype::real, 1>();
  }

  scheme_type scheme;
  scheme.require(model);

  prtcl::rt::neighbourhood<prtcl::rt::grouped_uniform_grid<model_policy>> nhood;

  nhood.load(model);
  scheme.load(model);

  nhood.update();
  scheme.test_counting_particles(nhood);
  scheme.test_counting_neighbors(nhood);

  int target_gpc = 789 + 123;

  CHECK(
      target_gpc ==
      model.get_global<nd_dtype::integer>("global_particle_count")[0]);
  CHECK(
      target_gpc * (123 + 456) ==
      model.get_global<nd_dtype::integer>("global_neighbor_count")[0]);
}
