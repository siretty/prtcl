#pragma once

#include "../meta/ctor_call_expand_pack.hpp"
#include "host/host_linear_access.hpp"
#include "host/host_linear_buffer.hpp"

#include <array>
#include <ostream>
#include <vector>

#include <cstddef> // size_t

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

  friend std::ostream &operator<<(std::ostream &s,
                                  array_of_vectors_data const &) {
    return s << "array_of_vectors_data";
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

  friend std::ostream &operator<<(std::ostream &s,
                                  array_of_vectors_buffer const &) {
    return s << "array_of_vectors_buffer";
  }
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
  template <typename, typename> struct get_impl_type;
  template <typename Value, size_t... Ns>
  struct get_impl_type<Value, std::index_sequence<Ns...>> {
    Value operator()(array_of_vectors_access const &s, size_t pos) const {
      return Value{s.data_[Ns][pos]...};
    }
  };
  template <typename Value>
  constexpr static get_impl_type<Value, std::make_index_sequence<N>> get_impl;

  template <typename, typename> struct set_impl_type;
  template <typename Value, size_t... Ns>
  struct set_impl_type<Value, std::index_sequence<Ns...>> {
    void operator()(array_of_vectors_access const &s, size_t pos,
                    Value const &value) const {
      ((void)(s.data_[Ns][pos] = value[Ns]), ...);
    }
  };
  template <typename Value>
  constexpr static set_impl_type<Value, std::make_index_sequence<N>> set_impl;

public:
  template <typename Value = value_type> Value get(size_t pos) const {
    return get_impl<Value>(*this, pos);
  }

  template <typename Value = value_type, typename U = T,
            typename = std::enable_if_t<!std::is_const<U>::value>>
  void set(size_t pos, value_type const &value) const {
    set_impl<Value>(*this, pos, value);
  }

  friend std::ostream &operator<<(std::ostream &s,
                                  array_of_vectors_access const &) {
    return s << "array_of_vectors_access";
  }
};

// ============================================================
// get_buffer(array_of_vectors_data, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_buffer<array_of_vectors_data<T, N, Linear>, Args...> {
  using type =
      array_of_vectors_buffer<T, N, typename get_buffer<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_buffer<array_of_vectors_data<T, N, Linear>,
                               Args...>::type
get_buffer(array_of_vectors_data<T, N, Linear> const &data, Args &&... args) {
  typename result_of::get_buffer<array_of_vectors_data<T, N, Linear>,
                                 Args...>::type result;
  for (size_t n = 0; n < N; ++n)
    result.data_[n] = get_buffer(data.data_[n], std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(array_of_vectors_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_rw_access<array_of_vectors_buffer<T, N, Linear>, Args...> {
  using type =
      array_of_vectors_access<T, N,
                              typename get_rw_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_rw_access<array_of_vectors_buffer<T, N, Linear>,
                                  Args...>::type
get_rw_access(array_of_vectors_buffer<T, N, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_rw_access<array_of_vectors_buffer<T, N, Linear>,
                                        Args...>::type;
  return ctor_call_expand_pack<result_type, N>(
      [](auto n, auto &buffer, auto &&... args) {
        return get_rw_access(buffer.data_[n],
                             std::forward<decltype(args)>(args)...);
      },
      buffer, std::forward<Args>(args)...);
}

// ============================================================
// get_ro_access(array_of_vectors_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_ro_access<array_of_vectors_buffer<T, N, Linear>, Args...> {
  using type =
      array_of_vectors_access<T, N,
                              typename get_ro_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_ro_access<array_of_vectors_buffer<T, N, Linear>,
                                  Args...>::type
get_ro_access(array_of_vectors_buffer<T, N, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_ro_access<array_of_vectors_buffer<T, N, Linear>,
                                        Args...>::type;
  return ctor_call_expand_pack<result_type, N>(
      [](auto n, auto &buffer, auto &&... args) {
        return get_ro_access(buffer.data_[n],
                             std::forward<decltype(args)>(args)...);
      },
      buffer, std::forward<Args>(args)...);
}

} // namespace prtcl
