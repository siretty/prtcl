#ifndef PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP
#define PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP

#include "../errors/not_implemented_error.hpp"
#include "../log.hpp"
#include "component_type.hpp"
#include "shape.hpp"

#include <iosfwd>
#include <string_view>

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

  friend std::ostream &operator<<(std::ostream &, TensorType const &);

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

#define PRTCL_DISPATCH_TTYPE_IMPL_SHAPE1_CASE(N0_, FUNCT_, ...)                \
  case N0_:                                                                    \
    FUNCT_<T, N0_>(__VA_ARGS__);                                               \
    break;

#define PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(N0_, N1_, FUNCT_, ...)           \
  else if (shape[0] == N0_ and shape[1] == N1_) {                              \
    FUNCT_<T, N0_, N1_>(__VA_ARGS__);                                          \
  }

#define PRTCL_DISPATCH_TTYPE_IMPL_SHAPE(FUNCT_, ...)                           \
  switch (shape.GetRank()) {                                                   \
  case 0:                                                                      \
    log::Debug("lib", "PRTCL_DISPATCH_TENSOR_TYPE", "matched rank=0");         \
    FUNCT_<T>(__VA_ARGS__);                                                    \
    break;                                                                     \
  case 1:                                                                      \
    log::Debug("lib", "PRTCL_DISPATCH_TENSOR_TYPE", "matched rank=1");         \
    switch (shape[0]) {                                                        \
      PRTCL_DISPATCH_TTYPE_IMPL_SHAPE1_CASE(1, FUNCT_, __VA_ARGS__)            \
      PRTCL_DISPATCH_TTYPE_IMPL_SHAPE1_CASE(2, FUNCT_, __VA_ARGS__)            \
      PRTCL_DISPATCH_TTYPE_IMPL_SHAPE1_CASE(3, FUNCT_, __VA_ARGS__)            \
    default:                                                                   \
      throw NotImplementedError{};                                             \
    }                                                                          \
    break;                                                                     \
  case 2:                                                                      \
    log::Debug("lib", "PRTCL_DISPATCH_TENSOR_TYPE", "matched rank=2");         \
    if (false) {                                                               \
      /* dummy, just to get else if's for the cases */                         \
    }                                                                          \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(1, 1, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(2, 1, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(3, 1, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(1, 2, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(2, 2, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(3, 2, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(1, 3, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(2, 3, FUNCT_, __VA_ARGS__)           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE2_CASE(3, 3, FUNCT_, __VA_ARGS__)           \
    else {                                                                     \
      throw NotImplementedError{};                                             \
    }                                                                          \
    break;                                                                     \
  default:                                                                     \
    throw NotImplementedError{};                                               \
  }

#define PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(CTYPE_, TYPE_, FUNCT_, ...)       \
  else if (ctype == ComponentType::CTYPE_) {                                   \
    using T = TYPE_;                                                           \
    PRTCL_DISPATCH_TTYPE_IMPL_SHAPE(FUNCT_, __VA_ARGS__)                       \
  }

#define PRTCL_DISPATCH_TENSOR_TYPE(TTYPE_, FUNCT_, ...)                        \
  {                                                                            \
    auto const ctype = (TTYPE_).GetComponentType();                            \
    auto const shape = (TTYPE_).GetShape();                                    \
    log::Debug(                                                                \
        "lib", "PRTCL_DISPATCH_TENSOR_TYPE", "ttype.rank=", shape.GetRank(),   \
        " ctype=", ctype.ToStringView());                                      \
    if (ctype == ComponentType::kInvalid) {                                    \
      throw NotImplementedError{};                                             \
    }                                                                          \
    PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(kBoolean, bool, FUNCT_, __VA_ARGS__)  \
    PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(                                      \
        kSInt32, int32_t, FUNCT_, __VA_ARGS__)                                 \
    PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(                                      \
        kSInt64, int64_t, FUNCT_, __VA_ARGS__)                                 \
    PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(kFloat32, float, FUNCT_, __VA_ARGS__) \
    PRTCL_DISPATCH_TTYPE_IMPL_CTYPE_CASE(                                      \
        kFloat64, double, FUNCT_, __VA_ARGS__)                                 \
    else {                                                                     \
      throw NotImplementedError{};                                             \
    }                                                                          \
  }

#endif // PRTCL_SRC_PRTCL_DATA_TENSOR_TYPE_HPP
