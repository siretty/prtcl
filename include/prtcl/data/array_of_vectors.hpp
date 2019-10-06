#pragma once

#include <array>
#include <vector>

#include <cstddef> // size_t

#include "detail/ctor_call_expand_pack.hpp"

namespace prtcl {

// ============================================================
// array_of_vectors_data<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear> struct array_of_vectors_data {
  std::array<Linear, N> data_;

public:
  size_t size() const { return data_[0].size(); }

  void resize(size_t new_size) {
    for (auto &v : data_)
      v.resize(new_size);
  }
};

// ============================================================
// array_of_vectors_buffer<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear>
struct array_of_vectors_buffer {
  std::array<Linear, N> data_;

public:
  size_t size() const { return data_[0].size(); }
};

// ============================================================
// array_of_vectors_acces<T, N, Linear>
// ============================================================

template <typename T, size_t N, typename Linear>
struct array_of_vectors_access {
  using value_type = std::array<std::remove_const_t<T>, N>;

public:
  std::array<Linear, N> data_;

public:
  size_t size() const { return data_[0].size(); }

private:
  template <typename> struct get_impl_type;
  template <size_t... Ns> struct get_impl_type<std::index_sequence<Ns...>> {
    value_type operator()(array_of_vectors_access const &s, size_t pos) const {
      return value_type{s.data_[Ns][pos]...};
    }
  };
  constexpr static get_impl_type<std::make_index_sequence<N>> get_impl;

  template <typename> struct set_impl_type;
  template <size_t... Ns> struct set_impl_type<std::index_sequence<Ns...>> {
    void operator()(array_of_vectors_access const &s, size_t pos,
                    value_type const &value) const {
      ((void)(s.data_[Ns][pos] = value[Ns]), ...);
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
// get_buffer(array_of_vectors_data, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_buffer(array_of_vectors_data<T, N, Linear> &data, Args &&... args) {
  array_of_vectors_buffer<T, N,
                          decltype(get_buffer(std::declval<Linear>(),
                                              std::forward<Args>(args)...))>
      result;
  for (size_t n = 0; n < N; ++n)
    result.data_[n] = get_buffer(data.data_[n], std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(array_of_vectors_buffer, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_rw_access(array_of_vectors_buffer<T, N, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      array_of_vectors_access<T, N,
                              decltype(
                                  get_rw_access(std::declval<Linear &>(),
                                                std::forward<Args>(args)...))>;
  return detail::ctor_call_expand_pack<result_type, N>(
      [](auto n, auto &buffer, auto &&... args) {
        return get_rw_access(buffer.data_[n],
                             std::forward<decltype(args)>(args)...);
      },
      buffer, std::forward<Args>(args)...);
}

// ============================================================
// get_ro_access(array_of_vectors_buffer, ...)
// ============================================================

template <typename T, size_t N, typename Linear, typename... Args>
auto get_ro_access(array_of_vectors_buffer<T, N, Linear> &buffer,
                   Args &&... args) {
  using result_type =
      array_of_vectors_access<T, N,
                              decltype(
                                  get_ro_access(std::declval<Linear &>(),
                                                std::forward<Args>(args)...))>;
  return detail::ctor_call_expand_pack<result_type, N>(
      [](auto n, auto &buffer, auto &&... args) {
        return get_ro_access(buffer.data_[n],
                             std::forward<decltype(args)>(args)...);
      },
      buffer, std::forward<Args>(args)...);
}

} // namespace prtcl
