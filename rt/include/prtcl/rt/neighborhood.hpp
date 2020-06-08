#pragma once

#include "common.hpp"

#include "basic_model.hpp"
#include "log/trace.hpp"

#include <iterator>
#include <vector>

#include <iostream>

#include <boost/container/flat_set.hpp>

namespace prtcl::rt {

template <typename GridType_> class neighbourhood {
  using grid_type = GridType_;

  using model_policy = typename grid_type::model_policy;
  using type_policy = typename model_policy::type_policy;
  using math_policy = typename model_policy::math_policy;
  using data_policy = typename model_policy::data_policy;

private:
  using model_type = basic_model<model_policy>;
  using group_type = basic_group<model_policy>;

  using real = typename type_policy::template dtype_t<dtype::real>;
  static constexpr size_t dimensionality = model_policy::dimensionality;

  template <dtype DType_, size_t... Ns_>
  using ndtype_data_ref_t =
      typename data_policy::template ndtype_data_ref_t<DType_, Ns_...>;

private:
  struct group_data_type {
    bool has_position;
    ndtype_data_ref_t<dtype::real, dimensionality> position;
    boost::container::flat_set<std::string> tags;

    bool has_tag(std::string const &tag) const { return tags.contains(tag); }

    friend auto get_element_count(group_data_type const &g_) {
      return g_.position.size();
    }

    friend decltype(auto)
    get_element_ref(group_data_type const &g_, size_t i_) {
      return g_.position[i_];
    }

    friend bool can_be_neighbor(group_data_type const &g_) {
      // NOTE: keep in sync with static can_be_neighbor(group_type const &)
      return not g_.tags.contains("cannot_be_neighbor");
    }
  };

  static bool can_be_neighbor(group_type const &g_) {
    // NOTE: keep in sync with group_data_type::can_be_neighbor
    return not g_.has_tag("cannot_be_neighbor");
  }

  struct model_data_type {
    std::vector<group_data_type> groups;

    friend auto get_group_count(model_data_type const &s_) {
      return s_.groups.size();
    }

    friend decltype(auto) get_group_ref(model_data_type const &s_, size_t i_) {
      return s_.groups[i_];
    }
  };

public:
  void rebuild(model_type &model) {
    PRTCL_RT_LOG_TRACE_SCOPED("neighorhood rebuild");

    auto radius = _grid.get_radius();
    _grid = grid_type{radius};

    load(model);
    update();
    permute(model);
  }

public:
  void set_radius(real radius_) {
    // TODO: automatically update when the radius changes
    _grid.set_radius(radius_);
  }

public:
  void load(model_type &model_) {
    PRTCL_RT_LOG_TRACE_SCOPED("neighorhood load");

    // resize the groups data
    _data.groups.resize(model_.groups().size());
    // iterate over all groups
    for (size_t i = 0; i < _data.groups.size(); ++i) {
      using difference_type =
          typename decltype(model_.groups())::difference_type;
      auto &group = model_.groups()[static_cast<difference_type>(i)];
      _data.groups[i].position = {};
      _data.groups[i].has_position =
          group.template has_varying<dtype::real, dimensionality>("position");
      _data.groups[i].tags.clear();
      _data.groups[i].tags.insert(
          boost::begin(group.tags()), boost::end(group.tags()));
      if (_data.groups[i].has_position) {
        _data.groups[i].position =
            group.template get_varying<dtype::real, dimensionality>("position");
      }
    }
  }

public:
  void update() {
    PRTCL_RT_LOG_TRACE_SCOPED("neighorhood update");

    _grid.update(_data);
  }

public:
  void permute(model_type &model_) {
    PRTCL_RT_LOG_TRACE_SCOPED("neighorhood permute");

    // resize all permutation storage
    _perm.resize(model_.groups().size());
    // resize the permutation iterators
    _perm_it.clear();
    _perm_it.reserve(_perm.size());
    // resize the permutation for each group and fetch the iterator
    for (size_t i = 0; i < model_.groups().size(); ++i) {
      _perm[i].clear();
      _perm[i].reserve(model_.groups()[static_cast<ssize_t>(i)].size());
      _perm_it.push_back(std::back_inserter(_perm[i]));
    }
    // compute all permutations
    _grid.compute_group_permutations(_perm_it);
#pragma omp parallel
    {
      for (size_t i = 0; i < model_.groups().size(); ++i) {
        auto &group = model_.groups()[static_cast<ssize_t>(i)];
        if (can_be_neighbor(group))
          group.permute(_perm[i]);
      }
    }
    // update the grid with the permuted positions
    _grid.update(_data);
  }

public:
  template <typename Fn> void neighbors(size_t g_, size_t i_, Fn &&fn) const {
#ifdef PRTCL_RT_NEIGHBORHOOD_DEBUG_SEARCH
    PRTCL_RT_LOG_TRACE_SCOPED("neighbor search", "g=", g_, " i=", i_);
#endif

    if (_data.groups[g_].has_position)
      _grid.neighbors(g_, i_, _data, std::forward<Fn>(fn));
  }

  template <typename X, typename Fn>
  void neighbors(X const &x_, Fn &&fn) const {
    _grid.neighbors(x_, _data, std::forward<Fn>(fn));
  }

private:
  grid_type _grid;
  model_data_type _data;
  std::vector<std::vector<size_t>> _perm;
  std::vector<std::back_insert_iterator<typename decltype(_perm)::value_type>>
      _perm_it;
};

} // namespace prtcl::rt
