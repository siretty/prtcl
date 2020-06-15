#pragma once

#include "../data/model.hpp"
#include "../log.hpp"
#include "../cxx.hpp"

#include <iterator>
#include <memory>
#include <vector>

#include <iostream>

#include <boost/container/flat_set.hpp>

namespace prtcl {

class NeighborhoodPImpl;

struct NeighborhoodPImplDeleter {
  void operator()(NeighborhoodPImpl *);
};

class Neighborhood {
public:
  Neighborhood();

public:
  void SetRadius(double radius_);

public:
  void Load(Model const &model_);

  void Update();

  void Permute(Model &model);

  void FindNeighbors(
      size_t g_, size_t i_, std::vector<std::vector<size_t>> &neighbors) const;

  /*
  template <typename X>
  void FindNeighbors(
      X const &x_, std::vector<std::vector<size_t>> &neighbors) const {
    log::Debug("lib", "Neighborhood", "FindNeighbors(..., ", &neighbors, ")");
    //_grid.neighbors(x_, _data, std::forward<Fn>(fn));
  }
   */

private:
  std::unique_ptr<NeighborhoodPImpl, NeighborhoodPImplDeleter> pimpl_;

  /*
  grid_type _grid;
  model_data_type _data;
  std::vector<std::vector<size_t>> _perm;
  std::vector<std::back_insert_iterator<typename decltype(_perm)::value_type>>
      _perm_it;
   */
};

} // namespace prtcl
