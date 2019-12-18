#pragma once

#include <utility>
#include <vector>

#include <Eigen/Eigen>

namespace prtcl::data {

// struct select_field_value_type<...> {{{
template <typename, typename> struct select_field_value_type;

template <typename T, typename I>
struct select_field_value_type<T, std::integer_sequence<I>> {
  using type = T;
};

template <typename T, typename I, I Rows>
struct select_field_value_type<T, std::integer_sequence<I, Rows>> {
  using type = Eigen::Matrix<T, static_cast<int>(Rows), 1>;
};

template <typename T, typename I, I Rows, I Cols>
struct select_field_value_type<T, std::integer_sequence<I, Rows, Cols>> {
  using type = Eigen::Matrix<T, static_cast<int>(Rows), static_cast<int>(Cols)>;
};
// }}}

template <typename Scalar, typename Shape> struct ndfield {
public:
  using element = Scalar;
  using shape_type = Shape;

public:
  using value_type =
      typename select_field_value_type<element, shape_type>::type;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

public:
  size_t size() const { return _data.size(); }

public:
  reference operator[](size_t index_) { return _data[index_]; }

  const_reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() { return _data.data(); }
  const_pointer data() const { return _data.data(); }

public:
  ndfield() = default;

  ndfield(ndfield const &) = default;
  ndfield &operator=(ndfield const &) = default;

public:
  void resize(size_t size_) { _data.resize(size_); }

  void resize(size_t size_, value_type const &value_) {
    _data.resize(size_, value_);
  }

  void clear() { resize(0); }

private:
  std::vector<value_type> _data;
};

template <typename T, size_t... Ns>
using ndfield_t = ndfield<T, std::index_sequence<Ns...>>;

template <typename Scalar, typename Shape> struct ndfield_ref {
public:
  using element = Scalar;
  using shape_type = Shape;

public:
  using value_type =
      typename select_field_value_type<element, shape_type>::type;
  using reference = value_type &;
  using pointer = value_type *;

public:
  size_t size() const { return _size; }

public:
  reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() const { return _data; }

public:
  ndfield_ref() = default;

  ndfield_ref(ndfield_ref const &) = default;
  ndfield_ref &operator=(ndfield_ref const &) = default;

  ndfield_ref(size_t size_, value_type *data_) : _size{size_}, _data{data_} {}

  explicit ndfield_ref(ndfield<element, shape_type> &owner_)
      : _size{owner_.size()}, _data{owner_.data()} {}

public:
  template <typename Range> static value_type from_range(Range range_) {
    if constexpr (0 < shape_type::size()) {
      value_type result;
      Eigen::Index index = 0;
      auto last = range_.end();
      for (auto it = range_.begin(); it != last; ++it, ++index)
        result[index] = static_cast<element>(*it);
      return result;
    } else
      return *range_.begin();
  }

private:
  size_t _size = 0;
  value_type *_data = nullptr;
};

template <typename T, size_t... Ns>
using ndfield_ref_t = ndfield_ref<T, std::index_sequence<Ns...>>;

} // namespace prtcl::data
