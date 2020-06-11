#ifndef PRTCL_VECTOR_OF_TENSORS_HPP
#define PRTCL_VECTOR_OF_TENSORS_HPP

#include "collection_of_mutable_tensors_impl.hpp"

#include "../math.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include <cstddef>

#include <boost/algorithm/apply_permutation.hpp>

#include <boost/operators.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>

namespace prtcl {

template <typename T, size_t... N>
class AccessToVectorOfTensors;

template <typename T, size_t... N>
class VectorOfTensors final : public CollectionOfMutableTensorsImpl<T, N...> {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  size_t GetSize() const final { return items_.size(); }

public:
  void Resize(size_t new_size) final {
    items_.resize(new_size, math::zeros<T, N...>());
  }

  void Permute(cxx::span<size_t> permutation) final {
    boost::algorithm::apply_permutation(items_, permutation);
  }

public:
  AccessToVectorOfTensors<T, N...> GetAccess() const;

public:
  friend void swap(VectorOfTensors &a, VectorOfTensors &b) {
    using std::swap;
    swap(a.items_, b.items_);
  }

private:
  mutable std::vector<ItemType> items_;

  friend class AccessToVectorOfTensors<T, N...>;
};

template <typename T, size_t... N>
class AccessToVectorOfTensors
    : public AccessToMutableTensorsImpl<T, N...>,
      public boost::equality_comparable<AccessToVectorOfTensors<T, N...>> {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  AccessToVectorOfTensors() = default;

  AccessToVectorOfTensors(VectorOfTensors<T, N...> const &owner)
      : size_{owner.GetSize()}, items_{owner.items_.data()} {}

public:
  size_t GetSize() const final { return size_; }

public:
  T GetComponent(size_t item, cxx::span<size_t const> cidx) const final {
    assert(item < GetSize());
    return math::At(items_[item], cidx);
  }

  using AccessToMutableTensorsImpl<T, N...>::GetComponent;

  void SetComponent(size_t item, cxx::span<size_t const> cidx, T value) final {
    assert(item < GetSize());
    math::At(items_[item], cidx) = value;
  }

  using AccessToMutableTensorsImpl<T, N...>::SetComponent;

public:
  ItemType const &GetItem(size_t index) const { return items_[index]; }

  template <typename Arg>
  void SetItem(size_t index, Arg &&arg) {
    items_[index] = std::forward<Arg>(arg);
  }

public:
  friend bool operator==(
      AccessToVectorOfTensors const &lhs, AccessToVectorOfTensors const &rhs) {
    return lhs.size_ == rhs.size_ and lhs.items_ == rhs.items_;
  }

  friend void swap(AccessToVectorOfTensors &a, AccessToVectorOfTensors &b) {
    using std::swap;
    swap(a.size_, b.size_);
    swap(a.items_, b.items_);
  }

private:
  size_t size_ = 0;
  ItemType *items_ = nullptr;
};

template <typename T, size_t... N>
AccessToVectorOfTensors<T, N...> VectorOfTensors<T, N...>::GetAccess() const {
  return {*this};
}

} // namespace prtcl

#endif // PRTCL_VECTOR_OF_TENSORS_HPP
