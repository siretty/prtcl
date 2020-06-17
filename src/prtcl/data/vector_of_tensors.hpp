#ifndef PRTCL_VECTOR_OF_TENSORS_HPP
#define PRTCL_VECTOR_OF_TENSORS_HPP

#include "collection_of_mutable_tensors_impl.hpp"

#include "../math.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <cstddef>

#include <boost/algorithm/apply_permutation.hpp>

#include <boost/container/small_vector.hpp>

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
  VectorOfTensors() = default;

  VectorOfTensors(VectorOfTensors const &) = delete;
  VectorOfTensors &operator=(VectorOfTensors const &) = delete;

  VectorOfTensors(VectorOfTensors &&) = default;
  VectorOfTensors &operator=(VectorOfTensors &&) = default;

public:
  size_t GetSize() const final { return items_.size(); }

public:
  void Resize(size_t new_size) final {
    items_.resize(new_size, math::zeros<T, N...>());
    access_ = GetAccessImpl();
  }

  void Permute(cxx::span<size_t> permutation) final {
    boost::algorithm::apply_permutation(items_, permutation);
  }

public:
  AccessToVectorOfTensors<T, N...> GetAccessImpl() const;

  AccessToMutableTensors const &GetAccess() const final { return access_; }

public:
  friend void swap(VectorOfTensors &a, VectorOfTensors &b) {
    using std::swap;
    swap(a.items_, b.items_);
    swap(a.access_, b.access_);
  }

private:
  mutable boost::container::small_vector<ItemType, 1> items_;
  AccessToVectorOfTensors<T, N...> access_ = {};

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

  void
  SetComponent(size_t item, cxx::span<size_t const> cidx, T value) const final {
    assert(item < GetSize());
    math::At(items_[item], cidx) = value;
  }

  using AccessToMutableTensorsImpl<T, N...>::SetComponent;

public:
  ItemType const &GetItem(size_t index) const { return items_[index]; }

  template <typename Arg>
  void SetItem(size_t index, Arg &&arg) const {
    items_[index] = std::forward<Arg>(arg);
  }

  ItemType &operator[](size_t index) const { return items_[index]; }

  // --------------------------------------------
  //  Experimental Accessor Methods

public:
#define PRTCL_DEFINE_ITEM_FROM_S(TYPE_)                                        \
  virtual void ItemFrom(size_t index, TYPE_ const &item) const {               \
    if constexpr (sizeof...(N) == 0) {                                         \
      if constexpr (std::is_same_v<T, TYPE_>)                                  \
        (*this)[index] = item;                                                 \
      else                                                                     \
        (*this)[index] = static_cast<T>(item);                                 \
    } else                                                                     \
      throw NotImplementedError{};                                             \
  }

#define PRTCL_DEFINE_ITEM_FROM_R(TYPE_, RANK_)                                 \
  virtual void ItemFrom(                                                       \
      size_t index, DynamicTensorT<TYPE_, RANK_> const &item) const {          \
    if constexpr (sizeof...(N) == RANK_) {                                     \
      if constexpr (std::is_same_v<T, TYPE_>)                                  \
        (*this)[index] = item;                                                 \
      else                                                                     \
        (*this)[index] = item.cast<T>();                                       \
    } else                                                                     \
      throw NotImplementedError{};                                             \
  }

#define PRTCL_DEFINE_ITEM_FROM(TYPE_)                                          \
  PRTCL_DEFINE_ITEM_FROM_S(TYPE_)                                              \
  PRTCL_DEFINE_ITEM_FROM_R(TYPE_, 1)                                           \
  PRTCL_DEFINE_ITEM_FROM_R(TYPE_, 2)

  PRTCL_DEFINE_ITEM_FROM(bool)
  PRTCL_DEFINE_ITEM_FROM(int32_t)
  PRTCL_DEFINE_ITEM_FROM(int64_t)
  PRTCL_DEFINE_ITEM_FROM(float)
  PRTCL_DEFINE_ITEM_FROM(double)

#undef PRTCL_DEFINE_ITEM_FROM_S
#undef PRTCL_DEFINE_ITEM_FROM_R
#undef PRTCL_DEFINE_ITEM_FROM

public:
#define PRTCL_DEFINE_ITEM_INTO_S(TYPE_)                                        \
  virtual void ItemInto(size_t index, TYPE_ &item) const {                     \
    if constexpr (sizeof...(N) == 0) {                                         \
      if constexpr (std::is_same_v<T, TYPE_>)                                  \
        item = (*this)[index];                                                 \
      else                                                                     \
        item = static_cast<TYPE_>((*this)[index]);                             \
    } else                                                                     \
      throw NotImplementedError{};                                             \
  }

#define PRTCL_DEFINE_ITEM_INTO_R(TYPE_, RANK_)                                 \
  virtual void ItemInto(size_t index, DynamicTensorT<TYPE_, RANK_> &item)      \
      const {                                                                  \
    if constexpr (sizeof...(N) == RANK_) {                                     \
      if constexpr (std::is_same_v<T, TYPE_>)                                  \
        item = (*this)[index];                                                 \
      else                                                                     \
        item = (*this)[index].template cast<TYPE_>();                          \
    } else                                                                     \
      throw NotImplementedError{};                                             \
  }

#define PRTCL_DEFINE_ITEM_INTO(TYPE_)                                          \
  PRTCL_DEFINE_ITEM_INTO_S(TYPE_)                                              \
  PRTCL_DEFINE_ITEM_INTO_R(TYPE_, 1)                                           \
  PRTCL_DEFINE_ITEM_INTO_R(TYPE_, 2)

  PRTCL_DEFINE_ITEM_INTO(bool)
  PRTCL_DEFINE_ITEM_INTO(int32_t)
  PRTCL_DEFINE_ITEM_INTO(int64_t)
  PRTCL_DEFINE_ITEM_INTO(float)
  PRTCL_DEFINE_ITEM_INTO(double)

#undef PRTCL_DEFINE_ITEM_INTO_S
#undef PRTCL_DEFINE_ITEM_INTO_R
#undef PRTCL_DEFINE_ITEM_INTO

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
AccessToVectorOfTensors<T, N...>
VectorOfTensors<T, N...>::GetAccessImpl() const {
  return {*this};
}

} // namespace prtcl

#endif // PRTCL_VECTOR_OF_TENSORS_HPP
