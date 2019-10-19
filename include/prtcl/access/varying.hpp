#pragma once

// TODO: merge with access::uniform as template specialization

#include "../meta/is_any_of.hpp"
#include "../tags.hpp"

#include <type_traits>
#include <utility>

#include <cstddef>

namespace prtcl::access {

template <typename AccessTag, typename Data> class varying {
  static_assert(tag::is_access_tag_v<AccessTag>, "AccessTag is invalid");

public:
  explicit varying(Data data) : _data{data} {}

  varying(varying const &) = default;
  varying &operator=(varying const &) = default;

  varying(varying &&) = default;
  varying &operator=(varying &&) = default;

public:
  template <
      typename Value, typename AT = AccessTag,
      typename = std::enable_if_t<is_any_of_v<AT, tag::read, tag::read_write>>>
  auto get(size_t index) const {
    return _data.template get<Value>(index);
  }

  template <typename Value, typename AT = AccessTag,
            typename = std::enable_if_t<is_any_of_v<AT, tag::read_write>>>
  auto set(size_t index, Value &&value) const {
    return _data.set(index, std::forward<Value>(value));
  }

  friend std::ostream &operator<<(std::ostream &s, varying const &v) {
    return s << "varying<" << AccessTag{} << ">{" << v._data << "}";
  }

private:
  Data _data;
};

template <typename AccessTag, typename Data> auto make_varying(Data &&data) {
  return varying<AccessTag, remove_cvref_t<Data>>{data};
}

} // namespace prtcl::access
