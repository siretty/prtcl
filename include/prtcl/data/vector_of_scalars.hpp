#pragma once

#include "host/host_linear_access.hpp"
#include "host/host_linear_buffer.hpp"

#include <array>
#include <ostream>
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

  void fill(value_type value) { std::fill(data_.begin(), data_.end(), value); }

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_scalars_data const &) {
    return s << "vector_of_scalars_data";
  }
};

// ============================================================
// vector_of_scalars_buffer<T, Linear>
// ============================================================

template <typename T, typename Linear> struct vector_of_scalars_buffer {
  using value_type = T;

  Linear data_;

public:
  size_t size() const { return data_.size(); }

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_scalars_buffer const &) {
    return s << "vector_of_scalars_buffer";
  }
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
  value_type set(size_t pos, value_type const &value) const {
    return data_[pos] = value;
  }

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_scalars_access const &) {
    return s << "vector_of_scalars_access";
  }
};

// ============================================================
// get_buffer(vector_of_scalars_data, ...)
// ============================================================

namespace result_of {

template <typename T, typename Linear, typename... Args>
struct get_buffer<vector_of_scalars_data<T, Linear>, Args...> {
  using type =
      vector_of_scalars_buffer<T, typename get_buffer<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, typename Linear, typename... Args>
typename result_of::get_buffer<vector_of_scalars_data<T, Linear>, Args...>::type
get_buffer(vector_of_scalars_data<T, Linear> const &data, Args &&... args) {
  typename result_of::get_buffer<vector_of_scalars_data<T, Linear>,
                                 Args...>::type result;
  result.data_ = get_buffer(data.data_, std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(vector_of_scalars_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, typename Linear, typename... Args>
struct get_rw_access<vector_of_scalars_buffer<T, Linear>, Args...> {
  using type =
      vector_of_scalars_access<T,
                               typename get_rw_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, typename Linear, typename... Args>
typename result_of::get_rw_access<vector_of_scalars_buffer<T, Linear>,
                                  Args...>::type
get_rw_access(vector_of_scalars_buffer<T, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_rw_access<vector_of_scalars_buffer<T, Linear>,
                                        Args...>::type;
  return result_type{get_rw_access(buffer.data_, std::forward<Args>(args)...)};
}

// ============================================================
// get_ro_access(vector_of_scalars_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, typename Linear, typename... Args>
struct get_ro_access<vector_of_scalars_buffer<T, Linear>, Args...> {
  using type =
      vector_of_scalars_access<T,
                               typename get_ro_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, typename Linear, typename... Args>
typename result_of::get_ro_access<vector_of_scalars_buffer<T, Linear>,
                                  Args...>::type
get_ro_access(vector_of_scalars_buffer<T, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_ro_access<vector_of_scalars_buffer<T, Linear>,
                                        Args...>::type;
  return result_type{get_ro_access(buffer.data_, std::forward<Args>(args)...)};
}

} // namespace prtcl

// vim: set foldmethod=marker:
