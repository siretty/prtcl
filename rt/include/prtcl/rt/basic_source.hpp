#pragma once

#include "common.hpp"

#include "nd_data_base.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <cstddef>

#include <boost/range/algorithm/copy.hpp>

#include <omp.h>

namespace prtcl::rt {

template <typename ModelPolicy_> class basic_source {
public:
  using model_policy = ModelPolicy_;

private:
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using data_policy = typename ModelPolicy_::data_policy;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_t =
      typename data_policy::template nd_dtype_data_t<DType_, Ns_...>;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t =
      typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

  static constexpr auto N = model_policy::dimensionality;

public:
  size_t size() const { return _spawn_positions->size(); }

public:
  void resize(size_t size_) { _spawn_positions->resize(size_); }

public:
  auto get_spawn_positions() const {
    return nd_dtype_data_ref_t<nd_dtype::real, N>{*_spawn_positions};
  }

public:
  auto get_initial_velocity() const {
    return nd_dtype_data_ref_t<nd_dtype::real, N>{*_initial_velocity};
  }

public:
  basic_source(basic_source &&) = default;
  basic_source &operator=(basic_source &&) = default;

  basic_source()
      : _spawn_positions{std::make_unique<
            nd_dtype_data_t<nd_dtype::real, N>>()},
        _initial_velocity{
            std::make_unique<nd_dtype_data_t<nd_dtype::real, N>>()} {
    // TODO: validate name and type
    _initial_velocity->resize(1);
  }

private:
  std::unique_ptr<nd_dtype_data_t<nd_dtype::real, N>> _spawn_positions;
  std::unique_ptr<nd_dtype_data_t<nd_dtype::real, N>> _initial_velocity;
};

} // namespace prtcl::rt
