#ifndef PRTCL_TENSORS_HPP
#define PRTCL_TENSORS_HPP

#include "tensor_type.hpp"

#include "../cxx.hpp"
#include "../math.hpp"

#include <any>
#include <typeindex>

#include <cstddef>
#include <cstdint>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/irange.hpp>

namespace prtcl {

class AccessToMutableTensors;

class CollectionOfMutableTensors {
public:
  virtual ~CollectionOfMutableTensors() = default;

public:
  virtual TensorType const &GetType() const = 0;

  virtual size_t GetSize() const = 0;

public:
  virtual void Resize(size_t new_size) = 0;

  virtual void Permute(cxx::span<size_t> mut_perm) = 0;

public:
  virtual AccessToMutableTensors const &GetAccess() const = 0;
};

class AccessToMutableTensors {
public:
  virtual ~AccessToMutableTensors() = default;

  // --------------------------------------------
  //  Abstract Methods

public:
  virtual TensorType const &GetType() const = 0;

  virtual size_t GetSize() const = 0;

public:
  virtual ComponentVariant
  GetComponentVariant(size_t item, cxx::span<size_t const> cidx) const = 0;

  virtual void SetComponentVariant(
      size_t item, cxx::span<size_t const> cidx,
      ComponentVariant value) const = 0;

  // --------------------------------------------
  //  Experimental Accessor Methods

public:
#define PRTCL_DECLARE_ITEM_FROM(TYPE_)                                         \
  virtual void ItemFrom(size_t index, DynamicTensorT<TYPE_, 0> const &item)    \
      const = 0;                                                               \
  virtual void ItemFrom(size_t index, DynamicTensorT<TYPE_, 1> const &item)    \
      const = 0;                                                               \
  virtual void ItemFrom(size_t index, DynamicTensorT<TYPE_, 2> const &item)    \
      const = 0;

  PRTCL_DECLARE_ITEM_FROM(bool)
  PRTCL_DECLARE_ITEM_FROM(int32_t)
  PRTCL_DECLARE_ITEM_FROM(int64_t)
  PRTCL_DECLARE_ITEM_FROM(float)
  PRTCL_DECLARE_ITEM_FROM(double)

#undef PRTCL_DECLARE_ITEM_FROM

public:
#define PRTCL_DECLARE_ITEM_INTO(TYPE_)                                         \
  virtual void ItemInto(size_t index, DynamicTensorT<TYPE_, 0> &item)          \
      const = 0;                                                               \
  virtual void ItemInto(size_t index, DynamicTensorT<TYPE_, 1> &item)          \
      const = 0;                                                               \
  virtual void ItemInto(size_t index, DynamicTensorT<TYPE_, 2> &item) const = 0;

  PRTCL_DECLARE_ITEM_INTO(bool)
  PRTCL_DECLARE_ITEM_INTO(int32_t)
  PRTCL_DECLARE_ITEM_INTO(int64_t)
  PRTCL_DECLARE_ITEM_INTO(float)
  PRTCL_DECLARE_ITEM_INTO(double)

#undef PRTCL_DECLARE_ITEM_INTO

  // --------------------------------------------
  //  Helper Methods

public:
  ComponentVariant
  GetComponentVariant(size_t item, std::initializer_list<size_t> cidx) const {
    return this->GetComponentVariant(item, cxx::span<size_t const>{cidx});
  }

  void SetComponentVariant(
      size_t item, std::initializer_list<size_t> cidx,
      ComponentVariant value) const {
    this->SetComponentVariant(item, cxx::span<size_t const>{cidx}, value);
  }
};

} // namespace prtcl

#endif // PRTCL_TENSORS_HPP
