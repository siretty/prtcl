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

  void CopyNeighbors(
      size_t g_, size_t i_, std::vector<std::vector<size_t>> &neighbors) const;

private:
  std::unique_ptr<NeighborhoodPImpl, NeighborhoodPImplDeleter> pimpl_;
};

} // namespace prtcl
