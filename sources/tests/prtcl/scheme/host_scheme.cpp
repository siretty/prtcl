#include <catch.hpp>

#include <prtcl/expr/field.hpp>
#include <prtcl/expr/group.hpp>
#include <prtcl/math/traits/host_math_traits.hpp>
#include <prtcl/scheme/host_scheme.hpp>

struct mock_settable_value {
  int get(size_t index_) const {
    this->get_index = index_;
    return value;
  }

  void set(size_t index_, int value_) const {
    this->set_index = index_;
    this->value = value_;
  }

  std::string name;
  mutable int value = 0;
  mutable size_t set_index = 0;
  mutable size_t get_index = 0;

  friend std::ostream &operator<<(std::ostream &s,
                                  mock_settable_value const &v) {
    return s << v.name;
  }
};

struct mock_gettable_value {
  int get(size_t index_) const {
    this->index = index_;
    return this->value;
  }

  std::string name;
  int value;
  mutable size_t index = 0;

  friend std::ostream &operator<<(std::ostream &s,
                                  mock_gettable_value const &v) {
    return s << v.name;
  }
};

TEST_CASE("prtcl/scheme/host_scheme", "[prtcl][scheme][host_scheme]") {
  using namespace prtcl;

  using math_traits = prtcl::host_math_traits<float, 3>;

  struct my_scheme : prtcl::host_scheme<my_scheme, math_traits> {
    void execute() {
      static auto const f = [](auto &g) { return g.has_flag("fluid"); };
      static auto const b = [](auto &g) { return g.has_flag("boundary"); };

      static expr::active_group const a;
      static expr::passive_group const p;

      static expr::vscalar<mock_settable_value> const vsa{{"a"}};
      static expr::vscalar<mock_gettable_value> const vsb{{"b", 56}};
      static expr::vscalar<mock_gettable_value> const vsc{{"c", 78}};

      this->for_each(f, vsa[a] = vsb[a], vsa[a] += vsc[a]);

      this->for_each(
          f, vsa[a] += vsb[a],
          this->for_each_neighbour(b, vsa[a] += vsb[p], vsa[a] += vsb[p]),
          vsa[a] = vsc[a]);
    }
  };

  my_scheme scheme;

  {
    auto gi = scheme.add_group();
    auto &gd = scheme.get_group(gi);
    gd.resize(3);
    gd.add_flag("fluid");
  }

  {
    auto gi = scheme.add_group();
    auto &gd = scheme.get_group(gi);
    gd.resize(4);
    gd.add_flag("boundary");
  }

  scheme.execute();
}

TEST_CASE("prtcl/scheme/host_scheme sesph",
          "[prtcl][scheme][host_scheme][sesph]") {
  using namespace prtcl;

  using math_traits = prtcl::host_math_traits<float, 3>;

  struct sesph_scheme : prtcl::host_scheme<sesph_scheme, math_traits> {
    expr::active_group const f, b;
    expr::passive_group const f_f, f_b, b_b;

    expr::uscalar<std::string> const m = {{"mass"}};
    expr::uscalar<std::string> const rho0 = {{"rest density"}};
    expr::uscalar<std::string> const kappa{{"compressibility"}};
    expr::uscalar<std::string> const nu{{"viscosity"}};

    expr::vvector<std::string> const x{{"position"}};
    expr::vvector<std::string> const v{{"velocity"}};
    expr::vvector<std::string> const a{{"acceleration"}};

    expr::vscalar<std::string> const V{{"volume"}};
    expr::vscalar<std::string> const rho{{"density"}};
    expr::vscalar<std::string> const p{{"pressure"}};

    expr::call_term<tag::kernel> const W;
    expr::call_term<tag::kernel_gradient> const GradW;

    expr::call_term<tag::max> const max;
    expr::call_term<tag::dot> const dot;
    expr::call_term<tag::norm_squared> const norm_sq;

    static auto fluid() {
      return [](auto &g) { return g.has_flag("fluid"); };
    }

    static auto boundary() {
      return [](auto &g) { return g.has_flag("boundary"); };
    }

    void prepare() {
      this->for_each(
          boundary(),
          // boundary volume
          V[b] = 0,
          this->for_each_neighbour(boundary(), V[b] += W(x[b] - x[b_b])),
          V[b] = 1 / V[b]);
    }

    void execute(scalar_type dt = 0.00001f) {
      this->update_neighbourhoods();

      auto const h = this->get_smoothing_scale();

      this->for_each(
          fluid(),
          // accumulate density
          rho[f] = 0,
          this->for_each_neighbour(fluid(), //
                                   rho[f] += m[f_f] * W(x[f] - x[f_f])),
          this->for_each_neighbour(boundary(), //
                                   rho[f] +=
                                   rho0[f] * V[f_b] * W(x[f] - x[f_b])),
          // compute pressure
          p[f] = kappa[f] * max(rho[f] / rho0[f] - 1, 0));

      // compute accelerations
      this->for_each(
          fluid(),
          this->for_each_neighbour(
              fluid(),
              // compute viscosity acceleration
              a[f] +=
              nu[f] * m[f_f] / rho[f] * dot(v[f] - v[f_f], x[f] - x[f_f]) /
              (norm_sq(x[f] - x[f_f]) + h * h / 100) * GradW(x[f] - x[f_f]),
              // compute pressure acceleration
              a[f] -=
              m[f_f] *
              (p[f] / (rho[f] * rho[f]) + p[f_f] / (rho[f_f] * rho[f_f])) *
              GradW(x[f] - x[f_f])),
          this->for_each_neighbour(
              boundary(),
              // compute viscosity acceleration
              a[f] +=
              nu[f] * rho0[f] * V[f_b] / rho[f] * dot(v[f], x[f] - x[f_b]) /
              (norm_sq(x[f] - x[f_b]) + h * h / 100) * GradW(x[f] - x[f_b]),
              // compute pressure acceleration
              a[f] -= rho0[f] * V[f_b] * (2 * p[f] / (rho[f] * rho[f])) *
                      GradW(x[f] - x[f_b])));

      // Euler-Cromer / Symplectic-Euler
      this->for_each(fluid(),
                     // compute velocity
                     v[f] += dt * a[f],
                     // compute position
                     x[f] += dt * v[f]);
    }
  };

  sesph_scheme scheme;
  scheme.set_smoothing_scale(0.025f);

  {
    auto gi = scheme.add_group();
    auto &gd = scheme.get_group(gi);
    gd.resize(3);
    gd.add_flag("fluid");

    gd.add_varying_vector("position");
    gd.add_varying_vector("velocity");
    gd.add_varying_vector("acceleration");

    gd.add_varying_scalar("density");
    gd.add_varying_scalar("pressure");

    gd.add_uniform_scalar("rest density");
    gd.add_uniform_scalar("compressibility");
  }

  {
    auto gi = scheme.add_group();
    auto &gd = scheme.get_group(gi);
    gd.resize(4);
    gd.add_flag("boundary");

    gd.add_varying_vector("position");

    gd.add_varying_scalar("volume");
  }

  // scheme.prepare();
  // scheme.execute();
}
