#pragma once

#include "array_of_vectors.hpp"
#include "host/host_linear_data.hpp"
#include "vector_of_scalars.hpp"

#include <optional>
#include <string>
#include <unordered_map>

#include <cstddef>

namespace prtcl {

template <typename T, size_t N> struct group_data {
public:
  using vectors_type = array_of_vectors_data<T, N, host_linear_data<T>>;
  using scalars_type = vector_of_scalars_data<T, host_linear_data<T>>;

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

  std::optional<scalars_type> get_varying_scalar(std::string name) {
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

  std::optional<vectors_type> get_varying_vector(std::string name) {
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
    uniform_scalars_data_.resize(index);
  }

  std::optional<size_t> get_uniform_scalar_index(std::string name) {
    if (auto it = uniform_scalars_.find(name); it != uniform_scalars_.end())
      return it->second;
    else
      return std::nullopt;
  }

  scalars_type get_uniform_scalars() { return &uniform_scalars_data_; }

private:
  std::unordered_map<std::string, size_t> uniform_vectors_;
  scalars_type uniform_vectors_data_;

public:
  void add_uniform_vector(std::string name) {
    size_t index = uniform_vectors_data_.size();
    auto [it, inserted] = uniform_vectors_.insert({name, index});
    if (!inserted)
      return;
    uniform_vectors_data_.resize(index);
  }

  std::optional<size_t> get_uniform_vector_index(std::string name) {
    if (auto it = uniform_vectors_.find(name); it != uniform_vectors_.end())
      return it->second;
    else
      return std::nullopt;
  }

  vectors_type get_uniform_vectors() { return &uniform_vectors_data_; }
};

} // namespace prtcl
