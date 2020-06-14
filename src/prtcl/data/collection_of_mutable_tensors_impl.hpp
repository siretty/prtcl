#ifndef PRTCL_SRC_PRTCL_DATA_COLLECTION_OF_MUTABLE_TENSORS_IMPL_HPP
#define PRTCL_SRC_PRTCL_DATA_COLLECTION_OF_MUTABLE_TENSORS_IMPL_HPP

#include "collection_of_mutable_tensors.hpp"

namespace prtcl {

// ============================================
//  Owning Type

template <typename T, size_t... N>
class CollectionOfMutableTensorsImpl : public CollectionOfMutableTensors {
  // --------------------------------------------
  //  Final Methods

public:
  TensorType const &GetType() const final {
    return GetTensorTypeCRef<T, N...>();
  }
};

// ============================================
//  Reference Type

template <typename T, size_t... N>
class AccessToMutableTensorsImpl : public AccessToMutableTensors {
  // --------------------------------------------
  //  Abstract Methods

public:
  virtual T GetComponent(size_t item, cxx::span<size_t const> cidx) const = 0;

  virtual void
  SetComponent(size_t item, cxx::span<size_t const> cidx, T value) const = 0;

  // --------------------------------------------
  //  Final Methods

public:
  TensorType const &GetType() const final {
    return GetTensorTypeCRef<T, N...>();
  }

public:
  ComponentVariant
  GetComponentVariant(size_t item, cxx::span<size_t const> cidx) const final {
    return {GetComponent(item, cidx)};
  }

  void SetComponentVariant(
      size_t item, cxx::span<size_t const> cidx,
      ComponentVariant value) const final {
    SetComponent(item, cidx, std::get<T>(value));
  }

  // --------------------------------------------
  //  Helper Methods

public:
  T GetComponent(size_t item, std::initializer_list<size_t> cidx) const {
    return this->GetComponent(item, cxx::span<size_t const>{cidx});
  }

  void
  SetComponent(size_t item, std::initializer_list<size_t> cidx, T value) const {
    return this->SetComponent(item, cxx::span<size_t const>{cidx}, value);
  }

  using AccessToMutableTensors::GetComponentVariant;
  using AccessToMutableTensors::SetComponentVariant;
};

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_COLLECTION_OF_MUTABLE_TENSORS_IMPL_HPP
