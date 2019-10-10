#pragma once

#include "../meta/ctor_call_expand_pack.hpp"
#include "host/host_linear_access.hpp"
#include "host/host_linear_buffer.hpp"

#include <array>
#include <ostream>
#include <type_traits>
#include <vector>

#include <cstddef> // size_t

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

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_arrays_data const &) {
    return s << "vector_of_arrays_data";
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

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_arrays_buffer const &) {
    return s << "vector_of_arrays_buffer";
  }
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
  template <typename, typename> struct get_impl_type;
  template <typename Value, size_t... Ns>
  struct get_impl_type<Value, std::index_sequence<Ns...>> {
    Value operator()(vector_of_arrays_access const &s, size_t pos) const {
      return Value{s.data_[pos * N + Ns]...};
    }
  };
  template <typename Value>
  constexpr static get_impl_type<Value, std::make_index_sequence<N>> get_impl =
      {};

  template <typename, typename> struct set_impl_type;
  template <typename Value, size_t... Ns>
  struct set_impl_type<Value, std::index_sequence<Ns...>> {
    void operator()(vector_of_arrays_access const &s, size_t pos,
                    Value const &value) const {
      ((void)(s.data_[pos * N + Ns] = value[Ns]), ...);
    }
  };
  template <typename Value>
  constexpr static set_impl_type<Value, std::make_index_sequence<N>> set_impl =
      {};

public:
  template <typename Value = value_type> Value get(size_t pos) const {
    return get_impl<Value>(*this, pos);
  }

  template <typename Value = value_type, typename U = T,
            typename = std::enable_if_t<!std::is_const<U>::value>>
  void set(size_t pos, Value const &value) const {
    set_impl<Value>(*this, pos, value);
  }

  friend std::ostream &operator<<(std::ostream &s,
                                  vector_of_arrays_access const &) {
    return s << "vector_of_arrays_access";
  }
};

// ============================================================
// get_buffer(vector_of_arrays_data, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_buffer<vector_of_arrays_data<T, N, Linear>, Args...> {
  using type =
      vector_of_arrays_buffer<T, N, typename get_buffer<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_buffer<vector_of_arrays_data<T, N, Linear>,
                               Args...>::type
get_buffer(vector_of_arrays_data<T, N, Linear> const &data, Args &&... args) {
  typename result_of::get_buffer<vector_of_arrays_data<T, N, Linear>,
                                 Args...>::type result;
  result.size_ = data.size_;
  result.data_ = get_buffer(data.data_, std::forward<Args>(args)...);
  return result;
}

// ============================================================
// get_rw_access(vector_of_arrays_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_rw_access<vector_of_arrays_buffer<T, N, Linear>, Args...> {
  using type =
      vector_of_arrays_access<T, N,
                              typename get_rw_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_rw_access<vector_of_arrays_buffer<T, N, Linear>,
                                  Args...>::type
get_rw_access(vector_of_arrays_buffer<T, N, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_rw_access<vector_of_arrays_buffer<T, N, Linear>,
                                        Args...>::type;
  return result_type{buffer.size(),
                     get_rw_access(buffer.data_, std::forward<Args>(args)...)};
}

// ============================================================
// get_ro_access(vector_of_arrays_buffer, ...)
// ============================================================

namespace result_of {

template <typename T, size_t N, typename Linear, typename... Args>
struct get_ro_access<vector_of_arrays_buffer<T, N, Linear>, Args...> {
  using type =
      vector_of_arrays_access<T, N,
                              typename get_ro_access<Linear, Args...>::type>;
};

} // namespace result_of

template <typename T, size_t N, typename Linear, typename... Args>
typename result_of::get_ro_access<vector_of_arrays_buffer<T, N, Linear>,
                                  Args...>::type
get_ro_access(vector_of_arrays_buffer<T, N, Linear> const &buffer,
              Args &&... args) {
  using result_type =
      typename result_of::get_ro_access<vector_of_arrays_buffer<T, N, Linear>,
                                        Args...>::type;
  return result_type{buffer.size(),
                     get_ro_access(buffer.data_, std::forward<Args>(args)...)};
}

} // namespace prtcl
