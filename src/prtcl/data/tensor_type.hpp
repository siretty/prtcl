#ifndef PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP
#define PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP

#include "component_type.hpp"
#include "shape.hpp"

#include <boost/range/numeric.hpp>

#include <boost/operators.hpp>

namespace prtcl {

class TensorType : public boost::equality_comparable<TensorType> {
public:
  TensorType() = default;

  TensorType(ComponentType ctype, Shape shape = {})
      : ctype_{ctype}, shape_{shape} {}

public:
  ComponentType const &GetComponentType() const { return ctype_; }

  Shape const &GetShape() const { return shape_; }

public:
  size_t GetComponentCount() const {
    return shape_.IsEmpty()
               ? 0
               : boost::accumulate(
                     shape_.GetExtents(), size_t{1}, std::multiplies<void>{});
  }

public:
  bool IsValid() const { return ctype_.IsValid(); }

  bool IsEmpty() const { return shape_.IsEmpty(); }

public:
  TensorType WithComponentType(ComponentType const &ctype) const {
    return {ctype, shape_};
  }

  TensorType WithShape(Shape const &shape) const { return {ctype_, shape}; }

  TensorType WithComponentTypeOf(TensorType const &other) const {
    return {other.ctype_, shape_};
  }

  TensorType WithShapeOf(TensorType const &other) const {
    return {ctype_, other.shape_};
  }

public:
  friend bool operator==(TensorType const &lhs, TensorType const &rhs) {
    return lhs.ctype_ == rhs.ctype_ and lhs.shape_ == rhs.shape_;
  }

private:
  ComponentType ctype_ = {};
  Shape shape_ = {};
};

template <typename T, size_t... N>
TensorType const &GetTensorTypeCRef() {
  static TensorType const ttype = {MakeComponentType<T>(), {N...}};
  return ttype;
}

template <typename T, size_t... N>
TensorType const &MakeTensorType() {
  return GetTensorTypeCRef<T, N...>();
}

} // namespace prtcl

#endif // PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP
