#pragma once

#include <array>
#include <type_traits>
#include <vector>

#include <cstddef> // size_t

namespace prtcl {

// ============================================================
// vector_of_scalars_data<T, Linear>
// ============================================================

template <typename T, typename Linear> struct vector_of_scalars_data {
  using value_type = T;

  Linear data_;

public:
  size_t size() const { return data_.size(); }

  void resize(size_t new_size) { data_.resize(new_size); }
};

// ============================================================
// vector_of_scalars_buffer<T, Linear>
// ============================================================

template <typename T, typename Linear> struct vector_of_scalars_buffer {
  using value_type = T;

  Linear data_;

public:
  size_t size() const { return data_.size(); }
};

// ============================================================
// vector_of_scalars_access<T, Linear>
// ============================================================

template <typename T, typename Linear> struct vector_of_scalars_access {
  using value_type = T;

  Linear data_;

public:
  size_t size() const { return data_.size(); }

public:
  value_type get(size_t pos) const { return data_[pos]; }

  template <typename U = T,
            typename = std::enable_if_t<!std::is_const<U>::value>>
  void set(size_t pos, value_type const &value) const {
    data_[pos] = value;
  }
};

// ============================================================
// get_buffer(vector_of_scalars_data, ...)
// ============================================================

template <typename T, typename Linear, typename... Args>
auto get_buffer(vector_of_scalars_data<T, Linear> &data, Args &&... args) {
  vector_of_scalars_buffer<T, decltype(get_buffer(std::declval<Linear &>(),
                                                  std::forward<Args>(args)...))>
      result;
  result.data_ = get_buffer(data.data_, std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(vector_of_scalars_buffer, ...)
// ============================================================

template <typename T, typename Linear, typename... Args>
auto get_rw_access(vector_of_scalars_buffer<T, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      vector_of_scalars_access<T, decltype(get_rw_access(
                                      std::declval<Linear &>(),
                                      std::forward<Args>(args)...))>;
  return result_type{get_rw_access(buffer.data_, std::forward<Args>(args)...)};
}

// ============================================================
// get_ro_access(vector_of_scalars_buffer, ...)
// ============================================================

template <typename T, typename Linear, typename... Args>
auto get_ro_access(vector_of_scalars_buffer<T, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      vector_of_scalars_access<T, decltype(get_ro_access(
                                      std::declval<Linear &>(),
                                      std::forward<Args>(args)...))>;
  return result_type{get_ro_access(buffer.data_, std::forward<Args>(args)...)};
}

} // namespace prtcl

// vim: set foldmethod=marker:
