#pragma once

#include "common.hpp"

#include "basic_group.hpp"

#include <boost/range/iterator_range_core.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <cstddef>

#include <boost/range/adaptor/transformed.hpp>

namespace prtcl::rt {

template <typename ModelPolicy_> class basic_model {
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
  using group_type = basic_group<ModelPolicy_>;

public:
  auto &add_group(std::string_view name_, std::string_view type_) {
    size_t index = _groups.size();
    auto [it, inserted] = _group_to_index.emplace(name_, index);
    (void)(inserted); // TODO
    return *_groups.emplace_back(new group_type{name_, type_});
  }

public:
  auto &get_group(size_t index_) const { return *_groups[index_].get(); }

  auto &get_group(std::string name_) const {
    if (auto it = _group_to_index.find(name_); it != _group_to_index.end())
      return get_group(it->second);
    else
      throw std::runtime_error{"unknown name"};
  }

public:
  auto groups() const {
    return _groups | boost::adaptors::transformed(
                         [](auto &ptr_) -> auto & { return *ptr_.get(); });
  }

public:
  template <nd_dtype DType_, size_t... Ns_> auto add_global(std::string name_) {
    auto *data = new nd_dtype_data_t<DType_, Ns_...>{};
    data->resize(1);
    auto [it, inserted] = _global.emplace(name_, data);
    (void)(inserted); // TODO
    return nd_dtype_data_ref_t<DType_, Ns_...>{*data};
  }

public:
  template <nd_dtype DType_, size_t... Ns_>
  auto get_global(std::string name_) const {
    if (auto it = _global.find(name_); it != _global.end())
      return nd_dtype_data_ref_t<DType_, Ns_...>{
          *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
    else
      throw std::runtime_error{"unknown name"};
  }

public:
  auto globals() const { return boost::make_iterator_range(_global); }

private:
  std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _global;
  std::unordered_map<std::string, size_t> _group_to_index;
  std::vector<std::unique_ptr<group_type>> _groups;
};

} // namespace prtcl::rt
