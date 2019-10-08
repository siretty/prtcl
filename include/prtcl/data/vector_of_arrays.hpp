#pragma once

#include <array>
#include <type_traits>
#include <vector>

#include <cstddef> // size_t

#include "detail/ctor_call_expand_pack.hpp"

namespace prtcl {

// ============================================================
// vector_of_arrays_data<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear> struct vector_of_arrays_data {
  using value_type = std::array<T, N>;

  size_t size_ = 0;
  Linear data_;

public:
  size_t size() const { return size_; }

  void resize(size_t new_size) {
    data_.resize(new_size * N);
    size_ = new_size;
  }
};

// ============================================================
// vector_of_arrays_buffer<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear>
struct vector_of_arrays_buffer {
  using value_type = std::array<T, N>;

  size_t size_ = 0;
  Linear data_;

public:
  size_t size() const { return size_; }
};

// ============================================================
// vector_of_arrays_access<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear>
struct vector_of_arrays_access {
  using value_type = std::array<T, N>;

  size_t size_ = 0;
  Linear data_;

public:
  size_t size() const { return size_; }

private:
  template <typename> struct get_impl_type;
  template <size_t... Ns> struct get_impl_type<std::index_sequence<Ns...>> {
    value_type operator()(vector_of_arrays_access const &s, size_t pos) const {
      return value_type{s.data_[pos * N + Ns]...};
    }
  };
  constexpr static get_impl_type<std::make_index_sequence<N>> get_impl;

  template <typename> struct set_impl_type;
  template <size_t... Ns> struct set_impl_type<std::index_sequence<Ns...>> {
    void operator()(vector_of_arrays_access const &s, size_t pos,
                    value_type const &value) const {
      ((void)(s.data_[pos * N + Ns] = value[Ns]), ...);
    }
  };
  constexpr static set_impl_type<std::make_index_sequence<N>> set_impl;

public:
  value_type get(size_t pos) const { return get_impl(*this, pos); }

  template <typename U = T,
            typename = std::enable_if_t<!std::is_const<U>::value>>
  void set(size_t pos, value_type const &value) const {
    set_impl(*this, pos, value);
  }
};

// ============================================================
// get_buffer(vector_of_arrays_data, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_buffer(vector_of_arrays_data<T, N, Linear> const &data,
                Args &&... args) {
  vector_of_arrays_buffer<T, N,
                          decltype(get_buffer(std::declval<Linear &>(),
                                              std::forward<Args>(args)...))>
      result;
  result.size_ = data.size_;
  result.data_ = get_buffer(data.data_, std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(vector_of_arrays_buffer, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_rw_access(vector_of_arrays_buffer<T, N, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      vector_of_arrays_access<T, N,
                              decltype(
                                  get_rw_access(std::declval<Linear &>(),
                                                std::forward<Args>(args)...))>;
  return result_type{buffer.size(),
                     get_rw_access(buffer.data_, std::forward<Args>(args)...)};
}

// ============================================================
// get_ro_access(vector_of_arrays_buffer, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_ro_access(vector_of_arrays_buffer<T, N, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      vector_of_arrays_access<T, N,
                              decltype(
                                  get_ro_access(std::declval<Linear &>(),
                                                std::forward<Args>(args)...))>;
  return result_type{buffer.size(),
                     get_ro_access(buffer.data_, std::forward<Args>(args)...)};
}

} // namespace prtcl
