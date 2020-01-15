#pragma once

#include "common.hpp"

#include "nd_data_base.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <cstddef>

namespace prtcl::rt {

template <typename ModelPolicy_> class basic_group {
  // {{{
  using type_policy = typename ModelPolicy_::type_policy;
  using math_policy = typename ModelPolicy_::math_policy;
  using data_policy = typename ModelPolicy_::data_policy;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_t =
      typename data_policy::template nd_dtype_data_t<DType_, Ns_...>;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t =
      typename data_policy::template nd_dtype_data_ref_t<DType_, Ns_...>;

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
    auto *data = new nd_dtype_data_t<DType_, Ns_...>{};
    data->resize(1); // uniform data has size 1
    auto [it, inserted] = _uniform.emplace(name_, data);
    (void)(inserted); // TODO
    return nd_dtype_data_ref_t<DType_, Ns_...>{*data};
  }

  template <nd_dtype DType_, size_t... Ns_>
  auto get_uniform(std::string name_) {
    if (auto it = _uniform.find(name_); it != _uniform.end())
      return nd_dtype_data_ref_t<DType_, Ns_...>{
          *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
    else
      throw std::runtime_error{"unknown name"};
  }

public:
  template <nd_dtype DType_, size_t... Ns_>
  auto add_varying(std::string name_) {
    auto data = new nd_dtype_data_t<DType_, Ns_...>{};
    data->resize(_size);
    auto [it, inserted] = _varying.emplace(name_, data);
    (void)(inserted); // TODO
    return nd_dtype_data_ref_t<DType_, Ns_...>{*data};
  }

  template <nd_dtype DType_, size_t... Ns_>
  auto get_varying(std::string name_) {
    if (auto it = _varying.find(name_); it != _varying.end())
      return nd_dtype_data_ref_t<DType_, Ns_...>{
          *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
    else
      throw std::runtime_error{"unknown name"};
  }

  template <nd_dtype DType_, size_t... Ns_>
  bool has_varying(std::string name_) {
    if (auto it = _varying.find(name_); it != _varying.end()) {
      nd_data_base *base = it->second.get();
      return base->dtype() == DType_ and base->shape() == nd_shape{Ns_...};
    } else
      return false;
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
  // }}}
};

} // namespace prtcl::rt
