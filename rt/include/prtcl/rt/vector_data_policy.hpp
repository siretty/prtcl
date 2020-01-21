#pragma once

#include "common.hpp"

#include "nd_data_base.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <cstddef>

#include <boost/algorithm/apply_permutation.hpp>

#include <boost/operators.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl::rt {

template <typename MathPolicy_, nd_dtype DType_, size_t... Ns_>
class vector_nd_data : public nd_data_base {
  // {{{
public:
  using value_type = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

public:
  nd_dtype dtype() const final { return DType_; }

  nd_shape shape() const final { return {Ns_...}; }

public:
  size_t size() const final { return _data.size(); }

public:
  reference operator[](size_t index_) { return _data[index_]; }

  const_reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() { return _data.data(); }
  const_pointer data() const { return _data.data(); }

public:
  vector_nd_data() = default;

  vector_nd_data(vector_nd_data const &) = default;
  vector_nd_data &operator=(vector_nd_data const &) = default;

public:
  void resize(size_t size_) final { _data.resize(size_); }

public:
  void permute(size_t *perm_) final {
    auto indices = boost::make_iterator_range(perm_, perm_ + size());
    boost::algorithm::apply_permutation(_data, indices);
  }

private:
  std::vector<value_type> _data;
  // }}}
};

template <typename MathPolicy_, nd_dtype DType_, size_t... Ns_>
class vector_nd_data_ref
    : public boost::equality_comparable<
          vector_nd_data_ref<MathPolicy_, DType_, Ns_...>> {
  // {{{
public:
  using value_type = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;
  using reference = value_type &;
  using pointer = value_type *;

public:
  nd_dtype dtype() const { return DType_; }

  nd_shape shape() const { return {Ns_...}; }

public:
  size_t size() const { return _size; }

public:
  reference operator[](size_t index_) const { return _data[index_]; }

public:
  pointer data() const { return _data; }

public:
  bool operator==(vector_nd_data_ref const &rhs_) const {
    return data() == rhs_.data() and size() == rhs_.size();
  }

public:
  vector_nd_data_ref() = default;

  vector_nd_data_ref(vector_nd_data_ref const &) = default;
  vector_nd_data_ref &operator=(vector_nd_data_ref const &) = default;

  vector_nd_data_ref(size_t size_, value_type *data_)
      : _size{size_}, _data{data_} {}

  explicit vector_nd_data_ref(
      vector_nd_data<MathPolicy_, DType_, Ns_...> &owner_)
      : _size{owner_.size()}, _data{owner_.data()} {}

public:
  // TODO
  // template <typename Range> static value_type from_range(Range range_) {
  //  if constexpr (0 < shape_type::size()) {
  //    value_type result;
  //    Eigen::Index index = 0;
  //    auto last = range_.end();
  //    for (auto it = range_.begin(); it != last; ++it, ++index)
  //      result[index] = static_cast<element>(*it);
  //    return result;
  //  } else
  //    return *range_.begin();
  //}

private:
  size_t _size = 0;
  value_type *_data = nullptr;
  // }}}
};

template <typename MathPolicy_> struct vector_data_policy {
private:
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_t = typename MathPolicy_::template nd_dtype_t<DType_, Ns_...>;

public:
  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_t = vector_nd_data<MathPolicy_, DType_, Ns_...>;

  template <nd_dtype DType_, size_t... Ns_>
  using nd_dtype_data_ref_t = vector_nd_data_ref<MathPolicy_, DType_, Ns_...>;
};

} // namespace prtcl::rt
