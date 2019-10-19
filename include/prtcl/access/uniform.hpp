#pragma once

// TODO: merge with access::varying as template specialization

#include "../meta/is_any_of.hpp"
#include "../meta/remove_cvref.hpp"
#include "../tags.hpp"

#include <type_traits>
#include <utility>
#include <ostream>

#include <cstddef>

namespace prtcl::access {

template <typename AccessTag, typename Data> class uniform {
  static_assert(tag::is_access_tag_v<AccessTag>, "AccessTag is invalid");

public:
  explicit uniform(Data data, size_t index) : _data{data}, _index{index} {}

  uniform(uniform const &) = default;
  uniform &operator=(uniform const &) = default;

  uniform(uniform &&) = default;
  uniform &operator=(uniform &&) = default;

public:
  template <
      typename Value, typename AT = AccessTag,
      typename = std::enable_if_t<is_any_of_v<AT, tag::read, tag::read_write>>>
  auto get(size_t) const {
    return _data.template get<Value>(_index);
  }

  template <typename Value, typename AT = AccessTag,
            typename = std::enable_if_t<is_any_of_v<AT, tag::read_write>>>
  auto set(size_t, Value &&value) const {
    return _data.set(_index, std::forward<Value>(value));
  }

  friend std::ostream & operator<<(std::ostream &s, uniform const &v) {
    return s << "uniform<" << AccessTag{} << ">{" << v._data << ", " << v._index << "}";
  }

private:
  Data _data;
  size_t _index;
};

template <typename AccessTag, typename Data>
auto make_uniform(Data &&data, size_t index) {
  return uniform<AccessTag, remove_cvref_t<Data>>{data, index};
}

} // namespace prtcl::access
