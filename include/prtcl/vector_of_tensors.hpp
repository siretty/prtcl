#ifndef PRTCL_VECTOR_OF_TENSORS_HPP
#define PRTCL_VECTOR_OF_TENSORS_HPP

#include <prtcl/math.hpp>
#include <prtcl/tensors.hpp>

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
class VectorOfTensorsRef;

template <typename T, size_t... N>
class VectorOfTensors final : public Tensors {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  size_t Size() const override { return items_.size(); }

  TypeID ComponentTypeID() const override { return {typeid(T)}; }

  ShapeType Shape() const override { return {N...}; }

public:
  ItemType const &GetItem(size_t index) const { return items_[index]; }

  template <typename Arg>
  void SetItem(size_t index, Arg &&arg) {
    items_[index] = std::forward<Arg>(arg);
  }

public:
  void Resize(size_t new_size) override {
    items_.resize(new_size, math::zeros<T, N...>());
  }

  void Permute(size_t *perm) override {
    auto indices = boost::make_iterator_range(perm, perm + Size());
    boost::algorithm::apply_permutation(items_, indices);
  }

public:
  friend void swap(VectorOfTensors &a, VectorOfTensors &b) {
    using std::swap;
    swap(a.items_, b.items_);
  }

private:
  std::vector<ItemType> items_;

  friend class VectorOfTensorsRef<T, N...>;
};

template <typename T, size_t... N>
class VectorOfTensorsRef
    : public boost::equality_comparable<VectorOfTensorsRef<T, N...>> {
public:
  using ItemType = math::Tensor<T, N...>;

public:
  VectorOfTensorsRef() = default;

  VectorOfTensorsRef(VectorOfTensors<T, N...> &owner)
      : size_{owner.Size()}, items_{owner.items_.data()} {}

public:
  size_t Size() const { return size_; }

public:
  ItemType const &GetItem(size_t index) const { return items_[index]; }

  template <typename Arg>
  void SetItem(size_t index, Arg &&arg) {
    items_[index] = std::forward<Arg>(arg);
  }

public:
  friend bool
  operator==(VectorOfTensorsRef const &lhs, VectorOfTensorsRef const &rhs) {
    return lhs.size_ == rhs.size_ and lhs.items_ == rhs.items_;
  }

  friend void swap(VectorOfTensorsRef &a, VectorOfTensorsRef &b) {
    using std::swap;
    swap(a.size_, b.size_);
    swap(a.items_, b.items_);
  }

private:
  size_t size_ = 0;
  ItemType *items_ = nullptr;
};

} // namespace prtcl

#endif // PRTCL_VECTOR_OF_TENSORS_HPP
