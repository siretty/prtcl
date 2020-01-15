#pragma once

#include "common.hpp"

#include "nd_data_base.hpp"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <cstddef>

#include <boost/operators.hpp>

#include <boost/range/iterator_range.hpp>

namespace prtcl::rt {

template <typename MathPolicy_, nd_dtype DType_, size_t... Ns_>
class vector_nd_data : public nd_data_base {
  // {{{
public:
  using value_type = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

public:
  nd_dtype dtype() const final { return DType_; }

  nd_shape shape() const final { return {Ns_...}; }

public:
  size_t size() const final { return _data.size(); }

public:
  reference operator[](size_t index_) { return _data[index_]; }

  const_reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() { return _data.data(); }
  const_pointer data() const { return _data.data(); }

public:
  vector_nd_data() = default;

  vector_nd_data(vector_nd_data const &) = default;
  vector_nd_data &operator=(vector_nd_data const &) = default;

public:
  void resize(size_t size_) final { _data.resize(size_); }

private:
  std::vector<value_type> _data;
  // }}}
};

template <typename MathPolicy_, nd_dtype DType_, size_t... Ns_>
class vector_nd_data_ref
    : public boost::equality_comparable<
          vector_nd_data_ref<MathPolicy_, DType_, Ns_...>> {
  // {{{
public:
  using value_type = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;
  using reference = value_type &;
  using pointer = value_type *;

public:
  nd_dtype dtype() const { return DType_; }

  nd_shape shape() const { return {Ns_...}; }

public:
  size_t size() const { return _size; }

public:
  reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() const { return _data; }

public:
  bool operator==(vector_nd_data_ref const &rhs_) const {
    return data() == rhs_.data() and size() == rhs_.size();
  }

public:
  vector_nd_data_ref() = default;

  vector_nd_data_ref(vector_nd_data_ref const &) = default;
  vector_nd_data_ref &operator=(vector_nd_data_ref const &) = default;

  vector_nd_data_ref(size_t size_, value_type *data_)
      : _size{size_}, _data{data_} {}

  explicit vector_nd_data_ref(
      vector_nd_data<MathPolicy_, DType_, Ns_...> &owner_)
      : _size{owner_.size()}, _data{owner_.data()} {}

public:
  // TODO
  // template <typename Range> static value_type from_range(Range range_) {
  //  if constexpr (0 < shape_type::size()) {
  //    value_type result;
  //    Eigen::Index index = 0;
  //    auto last = range_.end();
  //    for (auto it = range_.begin(); it != last; ++it, ++index)
  //      result[index] = static_cast<element>(*it);
  //    return result;
  //  } else
  //    return *range_.begin();
  //}

private:
  size_t _size = 0;
  value_type *_data = nullptr;
  // }}}
};

template <typename MathPolicy_> struct vector_data_policy {
private:
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_t = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;

public:
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_t = vector_nd_data<MathPolicy_, DType_, Ns_...>;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t = vector_nd_data_ref<MathPolicy_, DType_, Ns_...>;

public:
  class group_type {
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
      auto data = new nd_dtype_data_t<DType_, Ns_...>{};
      data->resize(1); // uniform data has size 1
      auto [it, inserted] = _uniform.emplace(name_, data);
      (void)(inserted); // TODO
      return nd_dtype_data_ref_t<DType_, Ns_...>{*data};
    }

    template <nd_dtype DType_, size_t... Ns_>
    auto add_varying(std::string name_) {
      auto data = new nd_dtype_data_t<DType_, Ns_...>{};
      data->resize(_size);
      auto [it, inserted] = _varying.emplace(name_, data);
      (void)(inserted); // TODO
      return nd_dtype_data_ref_t<DType_, Ns_...>{*data};
    }

  public:
    template <nd_dtype DType_, size_t... Ns_>
    auto get_uniform(std::string name_) {
      if (auto it = _uniform.find(name_); it != _uniform.end())
        return nd_dtype_data_ref_t<DType_, Ns_...>{
            *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
      else
        throw std::runtime_error{"unknown name"};
    }

    template <nd_dtype DType_, size_t... Ns_>
    auto get_varying(std::string name_) {
      if (auto it = _varying.find(name_); it != _varying.end())
        return nd_dtype_data_ref_t<DType_, Ns_...>{
            *static_cast<nd_dtype_data_t<DType_, Ns_...> *>(it->second.get())};
      else
        throw std::runtime_error{"unknown name"};
    }

  public:
    group_type() = delete;

    group_type(group_type &&) = default;
    group_type &operator=(group_type &&) = default;

    group_type(std::string_view name_, std::string_view type_)
        : _group_name{name_}, _group_type{type_} {
      // TODO: validate name and type
    }

  private:
    std::string _group_name;
    std::string _group_type;
    size_t _size = 0;
    std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _uniform;
    std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _varying;
  };

  class model_type {
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
        return _groups[it->second];
      else
        throw std::runtime_error{"unknown name"};
    }

  public:
    auto groups() const {
      return _groups | boost::adaptors::transformed(
                           [](auto &ptr_) -> auto & { return *ptr_.get(); });
    }

  public:
    template <nd_dtype DType_, size_t... Ns_>
    auto add_global(std::string name_) {
      auto data = new nd_dtype_data_t<DType_, Ns_...>{};
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

  private:
    std::unordered_map<std::string, std::unique_ptr<nd_data_base>> _global;
    std::unordered_map<std::string, size_t> _group_to_index;
    std::vector<std::unique_ptr<group_type>> _groups;
  };
};

} // namespace prtcl::rt
