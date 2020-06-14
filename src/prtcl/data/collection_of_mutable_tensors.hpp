#ifndef PRTCL_TENSORS_HPP
#define PRTCL_TENSORS_HPP

#include "tensor_type.hpp"

#include "../cxx.hpp"

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
