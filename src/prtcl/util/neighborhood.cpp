#include "neighborhood.hpp"

#include "../data/vector_of_tensors.hpp"
#include "../errors/not_implemented_error.hpp"
#include "grouped_uniform_grid.hpp"

#include <variant>

#include <boost/container/flat_set.hpp>

#include <boost/range/algorithm_ext/iota.hpp>

namespace prtcl {

namespace {

template <typename T, size_t N>
class NeighborhoodImpl {
  struct group_data_type {
    bool has_position;
    // AccessToVectorOfTensors<T, N> position;
    VaryingFieldSpan<T, N> position;
    boost::container::flat_set<std::string> tags;

    bool has_tag(std::string const &tag) const { return tags.contains(tag); }

    friend auto get_element_count(group_data_type const &group_data) {
      return group_data.position.GetSize();
    }

    friend decltype(auto)
    get_element_ref(group_data_type const &group_data, size_t item_index) {
      // return group_data.position.GetItem(item_index);
      return group_data.position[item_index];
    }

    friend bool can_be_neighbor(group_data_type const &group_data) {
      // NOTE: keep in sync with static can_be_neighbor(group_type const &)
      return not group_data.tags.contains("cannot_be_neighbor");
    }
  };

  struct model_data_type {
    std::vector<group_data_type> groups;

    friend auto get_group_count(model_data_type const &model_data) {
      return model_data.groups.size();
    }

    friend decltype(auto)
    get_group_ref(model_data_type const &model_data, size_t group_index) {
      return model_data.groups[group_index];
    }
  };

  static bool CanBeNeighbor(Group const &group) {
    // NOTE: keep in sync with group_data_type::can_be_neighbor
    return not group.HasTag("cannot_be_neighbor");
  }

public:
  void SetRadius(double radius) { _grid.set_radius(radius); }

  void Load(Model const &model_) {
    log::Debug("lib", "Neighborhood", "Load(", &model_, ")");

    // TODO: at the moment this will break if you remove a group

    // resize the groups data
    _data.groups.resize(model_.GetGroupCount());

    // iterate over all groups
    for (auto &group : model_.GetGroups()) {
      auto const group_index = static_cast<size_t>(group.GetGroupIndex());
      auto &group_data = _data.groups[group_index];

      try {
        group_data.position = group.GetVarying().FieldSpan<T, N>("position");
        group_data.has_position = true;
      } catch (...) {
        group_data.position = {};
        group_data.has_position = false;
      }
      // if (auto *field = group.GetVarying().TryGetFieldImpl<T, N>("position"))
      // {
      //  group_data.position = field->GetAccessImpl();
      //  group_data.has_position = true;
      //} else {
      //  group_data.position = {};
      //  group_data.has_position = false;
      //}

      group_data.tags.clear();
      group_data.tags.insert(
          boost::begin(group.GetTags()), boost::end(group.GetTags()));
    }
  }

  void Update() {
    log::Debug("lib", "Neighborhood", "Update");

    _grid.update(_data);
  }

  void Permute(Model &model) {
    log::Debug("lib", "Neighborhood", "Permute(", &model, ")");

    perm_it_.clear();

    // create the permutation and the iterator for each group
    for (auto &group : model.GetGroups()) {
      auto const group_index = static_cast<size_t>(group.GetGroupIndex());

      // resize the permutation storage
      if (perm_.size() <= group_index) {
        perm_.resize(group_index + 1);
        perm_it_.resize(group_index + 1);
      }

      // create the permutation for the group
      if (not perm_[group_index]) {
        perm_[group_index].reset(new std::vector<size_t>);
      }

      // resize it to the size of the group
      if (perm_[group_index]->size() != group.GetItemCount()) {
        perm_[group_index]->resize(group.GetItemCount());
        boost::range::iota(*perm_[group_index], size_t{0});
      }

      // store the back_insert_iterator
      perm_it_[group_index] = perm_[group_index]->data();

      // TODO: currently this never destroys the permutation for groups that are
      //       removed
    }

    // compute all permutations
    _grid.compute_group_permutations(perm_it_);

    // permute all groups
#pragma omp parallel
    {
      for (auto &group : model.GetGroups()) {
        auto const group_index = static_cast<size_t>(group.GetGroupIndex());
        if (CanBeNeighbor(group))
          group.Permute(*perm_[group_index]);
      }
    }

    // update the grid with the permuted positions
    Update();
  }

private:
  grouped_uniform_grid<N> _grid;
  model_data_type _data;

  std::vector<std::unique_ptr<std::vector<size_t>>> perm_;
  std::vector<size_t *> perm_it_;
};

} // namespace

class NeighborhoodPImpl {
public:
  void SetRadius(double radius) {
    std::visit(
        cxx::overloaded{
            [](std::monostate) { throw NotImplementedError{}; },
            [radius](auto *impl) { impl->SetRadius(radius); }},
        impl_);
  }

  void Load(Model const &model) {
    if (std::holds_alternative<std::monostate>(impl_)) {
      // TODO: HACK: IMPORTANT: this forces a particular ttype for position
      impl_ = new NeighborhoodImpl<float, 3>;
    }

    std::visit(
        cxx::overloaded{
            [](std::monostate) { throw NotImplementedError{}; },
            [&model](auto *impl) { impl->Load(model); }},
        impl_);
  }

  void Update() {
    std::visit(
        cxx::overloaded{
            [](std::monostate) { throw NotImplementedError{}; },
            [](auto *impl) { impl->Update(); }},
        impl_);
  }

  void Permute(Model &model) {
    std::visit(
        cxx::overloaded{
            [](std::monostate) { throw NotImplementedError{}; },
            [&model](auto *impl) { impl->Permute(model); }},
        impl_);
  }

private:
  std::variant<
      std::monostate, NeighborhoodImpl<float, 1> *,
      NeighborhoodImpl<float, 2> *, NeighborhoodImpl<float, 3> *,
      NeighborhoodImpl<double, 1> *, NeighborhoodImpl<double, 2> *,
      NeighborhoodImpl<double, 3> *>
      impl_ = {};
};

void NeighborhoodPImplDeleter::operator()(NeighborhoodPImpl *ptr) {
  delete ptr;
}

Neighborhood::Neighborhood() : pimpl_{new NeighborhoodPImpl} {}

void Neighborhood::SetRadius(double radius) { pimpl_->SetRadius(radius); }

void Neighborhood::Load(Model const &model) { pimpl_->Load(model); }

void Neighborhood::Update() { pimpl_->Update(); }

void Neighborhood::Permute(Model &model) { pimpl_->Permute(model); }

void Neighborhood::CopyNeighbors(
    size_t g_, size_t i_, std::vector<std::vector<size_t>> &neighbors) const {
  log::Debug(
      "lib", "Neighborhood", "CopyNeighbors(", g_, ", ", i_, ", ", &neighbors,
      ")");

  /*
  #ifdef PRTCL_RT_NEIGHBORHOOD_DEBUG_SEARCH
  PRTCL_RT_LOG_TRACE_SCOPED("neighbor search", "g=", g_, " i=", i_);
  #endif
   */

  // if (_data.groups[g_].has_position)
  //  _grid.neighbors(g_, i_, _data, std::forward<Fn>(fn));
}

} // namespace prtcl

/*

class Neighbourhood {
public:
  void Rebuild(Model const &model) {
    log::Debug("lib", "neighborhood", "rebuild");

     auto radius = _grid.get_radius();
    _grid = grid_type{radius};

     load(model);
     update();
     permute(model);
  }

public:
  void SetRadius(double radius_) {
    // TODO: automatically update when the radius changes
    _grid.set_radius(radius_);
  }

public:
  void Load(Model const &model_) {
    log::Debug("lib", "Neighborhood", "load");

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
void Update() {
  log::Debug("lib", "Neighborhood", "update");

  _grid.update(_data);
}

public:
void Permute(Model &model) {
  log::Debug("lib", "neighborhood", "permute")

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
template <typename Fn>
void FindNeighbors(size_t g_, size_t i_, Fn &&fn) const {
  log::Debug("lib", "Neighborhood", "FindNeighbors");

#ifdef PRTCL_RT_NEIGHBORHOOD_DEBUG_SEARCH
  PRTCL_RT_LOG_TRACE_SCOPED("neighbor search", "g=", g_, " i=", i_);
#endif

   if (_data.groups[g_].has_position)
    _grid.neighbors(g_, i_, _data, std::forward<Fn>(fn));
}

template <typename X, typename Fn>
void FindNeighbors(X const &x_, Fn &&fn) const {
  log::Debug("lib", "Neighborhood", "FindNeighbors");

  _grid.neighbors(x_, _data, std::forward<Fn>(fn));
}

private:
};

*/

/*

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

 */
