#pragma once

#include <prtcl/data/ndfield.hpp>
#include <prtcl/data/scheme.hpp>
#include <prtcl/expr/field.hpp>

#include <vector>

namespace prtcl {

template <typename Grid> class openmp_neighbourhood {
  using scalar_type = typename Grid::scalar_type;
  static constexpr size_t vector_extent = Grid::vector_extent;

private:
  using scheme_type = ::prtcl::data::scheme<scalar_type, vector_extent>;

private:
  struct group_data_type {
    ::prtcl::data::ndfield_ref_t<scalar_type, vector_extent> position;

    friend auto get_element_count(group_data_type const &g_) {
      return g_.position.size();
    }

    friend decltype(auto) get_element_ref(group_data_type const &g_,
                                          size_t i_) {
      return g_.position[i_];
    }
  };

  struct scheme_data_type {
    std::vector<group_data_type> groups;

    friend auto get_group_count(scheme_data_type const &s_) {
      return s_.groups.size();
    }

    friend decltype(auto) get_group_ref(scheme_data_type const &s_, size_t i_) {
      return s_.groups[i_];
    }
  };

public:
  void load(scheme_type &scheme_) {
    using namespace ::prtcl::expr_literals;
    _data.groups.resize(scheme_.get_group_count());
    for (auto [i, n, g] : scheme_.enumerate_groups())
      _data.groups[i].position = g.get("position"_vvf);
  }

  void set_radius(scalar_type radius_) { _grid.set_radius(radius_); }

public:
  void update() { _grid.update(_data); }

public:
  template <typename Fn> void neighbours(size_t g_, size_t i_, Fn &&fn) const {
    _grid.neighbours(g_, i_, _data, std::forward<Fn>(fn));
  }

private:
  Grid _grid;
  scheme_data_type _data;
};

} // namespace prtcl
