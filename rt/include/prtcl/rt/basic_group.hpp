#pragma once

#include "common.hpp"

#include "basic_source.hpp"
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

template <typename ModelPolicy_> class basic_group {
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

  using source_type = basic_source<model_policy>;

public:
  std::string_view get_name() const { return _group_name; }

public:
  std::string_view get_type() const { return _group_type; }

public:
  size_t size() const { return _size; }

public:
  void resize(size_t size_) {
    // resize all varying data
    for (auto &[name, data] : _varying)
      data->resize(size_);
    _size = size_;
  }

public:
  template <nd_dtype DType_, size_t... Ns_>
  auto add_uniform(std::string name_) {
    using data_type = nd_dtype_data_t<DType_, Ns_...>;
    auto data = std::make_unique<data_type>();
    data->resize(1);
    auto [it, inserted] = _uniform.emplace(name_, std::move(data));
    return nd_dtype_data_ref_t<DType_, Ns_...>{
        *static_cast<data_type *>(it->second.get())};
  }

  template <nd_dtype DType_, size_t... Ns_>
  auto get_uniform(std::string name_) const {
    if (auto it = _uniform.find(name_); it != _uniform.end())
      return nd_dtype_data_ref_t<DType_, Ns_...>{
          *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
    else
      throw std::runtime_error{"unknown name"};
  }

public:
  template <nd_dtype DType_, size_t... Ns_>
  auto add_varying(std::string name_) {
    using data_type = nd_dtype_data_t<DType_, Ns_...>;
    auto data = std::make_unique<data_type>();
    data->resize(_size);
    auto [it, inserted] = _varying.emplace(name_, std::move(data));
    return nd_dtype_data_ref_t<DType_, Ns_...>{
        *static_cast<data_type *>(it->second.get())};
  }

  template <nd_dtype DType_, size_t... Ns_>
  auto get_varying(std::string name_) const {
    if (auto it = _varying.find(name_); it != _varying.end())
      return nd_dtype_data_ref_t<DType_, Ns_...>{
          *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
    else
      throw std::runtime_error{"unknown name"};
  }

  template <nd_dtype DType_, size_t... Ns_>
  bool has_varying(std::string name_) const {
    if (auto it = _varying.find(name_); it != _varying.end()) {
      nd_data_base *base = it->second.get();
      return base->dtype() == DType_ and base->shape() == nd_shape{Ns_...};
    } else
      return false;
  }

public:
  auto &add_source() { return *_sources.emplace_back(new source_type{}); }

  auto all_sources() const { return boost::make_iterator_range(_sources); }

public:
  template <typename Range_> void permute(Range_ const &range_) {
#pragma omp single
    {
      // create indexed access to the varying data
      _to_permute.clear();
      _to_permute.reserve(_varying.size());
      for (auto const &[_, v] : _varying)
        _to_permute.push_back(v.get());
      // one permutation per thread
      _per_thread_perm.resize(static_cast<size_t>(omp_get_num_threads()));
    }
    // ensure that the current threads permutation has the right size
    auto tid = static_cast<size_t>(omp_get_thread_num());
    _per_thread_perm[tid].resize(boost::size(range_));
    // permute each varying field
#pragma omp for schedule(static, 1)
    for (size_t i = 0; i < _to_permute.size(); ++i) {
      boost::range::copy(range_, _per_thread_perm[tid].begin());
      _to_permute[i]->permute(_per_thread_perm[tid].data());
    }
  }

public:
  basic_group() = delete;

  basic_group(basic_group &&) = default;
  basic_group &operator=(basic_group &&) = default;

  basic_group(std::string_view name_, std::string_view type_)
      : _group_name{name_}, _group_type{type_} {
    // TODO: validate name and type
  }

private:
  std::string _group_name;
  std::string _group_type;
  size_t _size = 0;
  std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _uniform;
  std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _varying;
  std::vector<std::unique_ptr<source_type>> _sources;

  std::vector<std::vector<size_t>> _per_thread_perm;
  std::vector<nd_data_base *> _to_permute;
};

} // namespace prtcl::rt
