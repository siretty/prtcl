#pragma once

#include "array_of_vectors.hpp"
#include "host/host_linear_data.hpp"
#include "vector_of_scalars.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include <cstddef>

#include <iostream>

namespace prtcl {

namespace detail {
struct group_data_access;
} // namespace detail

template <typename T, size_t N, typename Linear = host_linear_data<T>>
struct group_data {
public:
  using vectors_type = array_of_vectors_data<T, N, Linear>;
  using scalars_type = vector_of_scalars_data<T, Linear>;

private:
  size_t size_ = 0;

public:
  size_t size() const { return size_; }

  void resize(size_t new_size) {
    for (auto &[_, v] : varying_vectors_)
      v.resize(new_size);

    for (auto &[_, s] : varying_scalars_)
      s.resize(new_size);

    size_ = new_size;
  }

private:
  std::unordered_map<std::string, scalars_type> varying_scalars_;

public:
  void add_varying_scalar(std::string name) {
    auto [it, inserted] = varying_scalars_.insert({name, scalars_type{}});
    if (inserted)
      it->second.resize(size());
  }

  std::optional<scalars_type> get_varying_scalar(std::string name) const {
    if (auto it = varying_scalars_.find(name); it != varying_scalars_.end())
      return it->second;
    else
      return std::nullopt;
  }

private:
  std::unordered_map<std::string, vectors_type> varying_vectors_;

public:
  void add_varying_vector(std::string name) {
    auto [it, inserted] = varying_vectors_.insert({name, vectors_type{}});
    if (inserted)
      it->second.resize(size());
  }

  std::optional<vectors_type> get_varying_vector(std::string name) const {
    if (auto it = varying_vectors_.find(name); it != varying_vectors_.end())
      return it->second;
    else
      return std::nullopt;
  }

private:
  std::unordered_map<std::string, size_t> uniform_scalars_;
  scalars_type uniform_scalars_data_;

public:
  void add_uniform_scalar(std::string name) {
    size_t index = uniform_scalars_data_.size();
    auto [it, inserted] = uniform_scalars_.insert({name, index});
    if (!inserted)
      return;
    uniform_scalars_data_.resize(index + 1);
  }

  std::optional<size_t> get_uniform_scalar_index(std::string name) const {
    if (auto it = uniform_scalars_.find(name); it != uniform_scalars_.end())
      return it->second;
    else
      return std::nullopt;
  }

  scalars_type get_uniform_scalars() const { return uniform_scalars_data_; }

private:
  std::unordered_map<std::string, size_t> uniform_vectors_;
  vectors_type uniform_vectors_data_;

public:
  void add_uniform_vector(std::string name) {
    size_t index = uniform_vectors_data_.size();
    auto [it, inserted] = uniform_vectors_.insert({name, index});
    if (!inserted)
      return;
    uniform_vectors_data_.resize(index + 1);
  }

  std::optional<size_t> get_uniform_vector_index(std::string name) const {
    if (auto it = uniform_vectors_.find(name); it != uniform_vectors_.end())
      return it->second;
    else
      return std::nullopt;
  }

  vectors_type get_uniform_vectors() const { return uniform_vectors_data_; }

  friend struct detail::group_data_access;
};

namespace detail {

struct group_data_access {
  template <typename T, size_t N, typename Linear>
  static auto get_varying_scalars(group_data<T, N, Linear> const &gd) {
    return std::make_pair(gd.varying_scalars_.begin(),
                          gd.varying_scalars_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto get_varying_vectors(group_data<T, N, Linear> const &gd) {
    return std::make_pair(gd.varying_vectors_.begin(),
                          gd.varying_vectors_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto get_uniform_scalar_indices(group_data<T, N, Linear> const &gd) {
    return std::make_pair(gd.uniform_scalars_.begin(),
                          gd.uniform_scalars_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto get_uniform_vector_indices(group_data<T, N, Linear> const &gd) {
    return std::make_pair(gd.uniform_vectors_.begin(),
                          gd.uniform_vectors_.end());
  }
};

} // namespace detail

namespace detail {
struct group_buffer_access;
} // namespace detail

template <typename T, size_t N, typename Linear = host_linear_data<T>>
struct group_buffer {
public:
  using vectors_type = array_of_vectors_buffer<T, N, Linear>;
  using scalars_type = vector_of_scalars_buffer<T, Linear>;

private:
  size_t size_ = 0;

public:
  size_t size() const { return size_; }

  void resize(size_t new_size) {
    for (auto &[_, v] : varying_vectors_)
      v.resize(new_size);

    for (auto &[_, s] : varying_scalars_)
      s.resize(new_size);

    size_ = new_size;
  }

private:
  std::unordered_map<std::string, scalars_type> varying_scalars_;

public:
  std::optional<scalars_type> get_varying_scalar(std::string name) const {
    if (auto it = varying_scalars_.find(name); it != varying_scalars_.end())
      return it->second;
    else
      return std::nullopt;
  }

private:
  std::unordered_map<std::string, vectors_type> varying_vectors_;

public:
  std::optional<vectors_type> get_varying_vector(std::string name) const {
    if (auto it = varying_vectors_.find(name); it != varying_vectors_.end())
      return it->second;
    else
      return std::nullopt;
  }

private:
  std::unordered_map<std::string, size_t> uniform_scalars_;
  scalars_type uniform_scalars_data_;

public:
  std::optional<size_t> get_uniform_scalar_index(std::string name) const {
    if (auto it = uniform_scalars_.find(name); it != uniform_scalars_.end())
      return it->second;
    else
      return std::nullopt;
  }

  scalars_type get_uniform_scalars() const { return uniform_scalars_data_; }

private:
  std::unordered_map<std::string, size_t> uniform_vectors_;
  vectors_type uniform_vectors_data_;

public:
  std::optional<size_t> get_uniform_vector_index(std::string name) const {
    if (auto it = uniform_vectors_.find(name); it != uniform_vectors_.end())
      return it->second;
    else
      return std::nullopt;
  }

  vectors_type get_uniform_vectors() const { return uniform_vectors_data_; }

  friend struct detail::group_buffer_access;
};

namespace detail {

struct group_buffer_access {
  template <typename T, size_t N, typename Linear>
  static auto set_size(group_buffer<T, N, Linear> &group, size_t size) {
    group.size_ = size;
  }

  template <typename T, size_t N, typename Linear>
  static auto get_varying_scalars(group_buffer<T, N, Linear> const &gd) {
    return std::make_pair(gd.varying_scalars_.begin(),
                          gd.varying_scalars_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto add_varying_scalar(
      group_buffer<T, N, Linear> &gd, std::string name,
      typename group_buffer<T, N, Linear>::scalars_type &&value) {
    gd.varying_scalars_.insert_or_assign(name, value);
  }

  template <typename T, size_t N, typename Linear>
  static auto get_varying_vectors(group_buffer<T, N, Linear> const &gd) {
    return std::make_pair(gd.varying_vectors_.begin(),
                          gd.varying_vectors_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto add_varying_vector(
      group_buffer<T, N, Linear> &gd, std::string name,
      typename group_buffer<T, N, Linear>::vectors_type &&value) {
    gd.varying_vectors_.insert_or_assign(name, value);
  }

  template <typename T, size_t N, typename Linear>
  static auto get_uniform_scalar_indices(group_buffer<T, N, Linear> const &gd) {
    return std::make_pair(gd.uniform_scalars_.begin(),
                          gd.uniform_scalars_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto set_uniform_scalars_data(
      group_buffer<T, N, Linear> &group,
      typename group_buffer<T, N, Linear>::scalars_type &&value) {
    group.uniform_scalars_data_ = value;
  }

  template <typename T, size_t N, typename Linear>
  static auto add_uniform_scalar(group_buffer<T, N, Linear> &gd,
                                 std::string name, size_t index) {
    gd.uniform_scalars_.insert_or_assign(name, index);
  }

  template <typename T, size_t N, typename Linear>
  static auto get_uniform_vector_indices(group_buffer<T, N, Linear> const &gd) {
    return std::make_pair(gd.uniform_vectors_.begin(),
                          gd.uniform_vectors_.end());
  }

  template <typename T, size_t N, typename Linear>
  static auto set_uniform_vectors_data(
      group_buffer<T, N, Linear> &group,
      typename group_buffer<T, N, Linear>::vectors_type &&value) {
    group.uniform_vectors_data_ = value;
  }

  template <typename T, size_t N, typename Linear>
  static auto add_uniform_vector(group_buffer<T, N, Linear> &gd,
                                 std::string name, size_t index) {
    gd.uniform_vectors_.insert_or_assign(name, index);
  }
};

} // namespace detail

// ============================================================
// get_buffer(array_of_vectors_data, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_buffer<group_data<T, N, Linear>, Args...> {
  using type = group_buffer<T, N, typename get_buffer<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_buffer<group_data<T, N, Linear>, Args...>::type
get_buffer(group_data<T, N, Linear> const &data, Args &&... args) {
  typename result_of::get_buffer<group_data<T, N, Linear>, Args...>::type
      result;
  // size
  detail::group_buffer_access::set_size(result, data.size());
  { // varying scalars
    auto [first, last] = detail::group_data_access::get_varying_scalars(data);
    for (auto it = first; it != last; ++it) {
      detail::group_buffer_access::add_varying_scalar(
          result, it->first,
          get_buffer(it->second, std::forward<Args>(args)...));
    }
  }
  { // varying vectors
    auto [first, last] = detail::group_data_access::get_varying_vectors(data);
    for (auto it = first; it != last; ++it) {
      detail::group_buffer_access::add_varying_vector(
          result, it->first,
          get_buffer(it->second, std::forward<Args>(args)...));
    }
  }
  { // uniform scalars
    detail::group_buffer_access::set_uniform_scalars_data(
        result,
        get_buffer(data.get_uniform_scalars(), std::forward<Args>(args)...));
    auto [first, last] =
        detail::group_data_access::get_uniform_scalar_indices(data);
    for (auto it = first; it != last; ++it) {
      detail::group_buffer_access::add_uniform_scalar(result, it->first,
                                                      it->second);
    }
  }
  { // uniform vectors
    detail::group_buffer_access::set_uniform_vectors_data(
        result,
        get_buffer(data.get_uniform_vectors(), std::forward<Args>(args)...));
    auto [first, last] =
        detail::group_data_access::get_uniform_vector_indices(data);
    for (auto it = first; it != last; ++it) {
      detail::group_buffer_access::add_uniform_vector(result, it->first,
                                                      it->second);
    }
  }
  return result;
}

} // namespace prtcl
