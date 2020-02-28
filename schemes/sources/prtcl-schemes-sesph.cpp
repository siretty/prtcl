#include <prtcl/rt/basic_application.hpp>

#include <prtcl/schemes/boundary.hpp>
#include <prtcl/schemes/sesph.hpp>
#include <prtcl/schemes/symplectic_euler.hpp>

#include <iostream>

template <typename ModelPolicy_>
class sesph_application final
    : public prtcl::rt::basic_application<ModelPolicy_> {
  using base_type = prtcl::rt::basic_application<ModelPolicy_>;

public:
  using model_policy = typename base_type::model_policy;
  using model_type = typename base_type::model_type;
  using neighborhood_type = typename base_type::neighborhood_type;

  using nd_dtype = prtcl::core::nd_dtype;

public:
  void on_require_schemes(model_type &model) override {
    require_all(model, boundary, sesph, advect);
  }

  void on_load_schemes(model_type &model) override {
    load_all(model, boundary, sesph, advect);

    for (auto &group : model.groups()) {
      if (group.get_type() == "boundary") {
        std::cerr << "BOUNDARY VISCOSITY: " << group.get_name() << ": "
                  << group.template get_uniform<nd_dtype::real>("viscosity")[0]
                  << std::endl;
      }
    }
  }

  void on_prepare_simulation(model_type &, neighborhood_type &nhood) override {
    boundary.compute_volume(nhood);
  }

  void on_step(model_type &, neighborhood_type &nhood) override {
    sesph.compute_density_and_pressure(nhood);
    sesph.compute_acceleration(nhood);
    advect.advect_symplectic_euler(nhood);
  }

  prtcl::schemes::boundary<model_policy> boundary;
  prtcl::schemes::sesph<model_policy> sesph;
  prtcl::schemes::symplectic_euler<model_policy> advect;
};

constexpr size_t N = 3;

using model_policy = prtcl::rt::basic_model_policy<
    prtcl::rt::fib_type_policy,
    prtcl::rt::mixed_math_policy<
        prtcl::rt::eigen_math_policy,
        prtcl::rt::kernel_math_policy_mixin<prtcl::rt::cubic_spline_kernel>>::
        template policy,
    prtcl::rt::vector_data_policy, N>;

int main(int argc_, char **argv_) {
  std::cerr << "PRESS [ENTER] TO START";
  std::getchar();

  sesph_application<model_policy> application;
  application.main(argc_, argv_);
}
