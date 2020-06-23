#ifndef PRTCL_MATH_HPP
#define PRTCL_MATH_HPP

#include "math/common.hpp"

#include <variant>

namespace prtcl {

template <typename T, size_t... N>
using TensorT = math::Tensor<T, N...>;

template <typename T, size_t R>
using DynamicTensorT = math::DynamicTensor<T, R>;

using BooleanScalar = math::DynamicTensor<bool, 0>;
using BooleanVector = math::DynamicTensor<bool, 1>;
using BooleanMatrix = math::DynamicTensor<bool, 2>;

using IntegerScalar = math::DynamicTensor<int64_t, 0>;
using IntegerVector = math::DynamicTensor<int64_t, 1>;
using IntegerMatrix = math::DynamicTensor<int64_t, 2>;

using RealScalar = math::DynamicTensor<double, 0>;
using RealVector = math::DynamicTensor<double, 1>;
using RealMatrix = math::DynamicTensor<double, 2>;

using ScalarVariant = std::variant<RealScalar, IntegerScalar, BooleanScalar>;
using VectorVariant = std::variant<RealVector, IntegerVector, BooleanVector>;
using MatrixVariant = std::variant<RealMatrix, IntegerMatrix, BooleanMatrix>;

using ValueVariant = std::variant<
    BooleanScalar, BooleanVector, BooleanMatrix, IntegerScalar, IntegerVector,
    IntegerMatrix, RealScalar, RealVector, RealMatrix>;

} // namespace prtcl

#endif // PRTCL_MATH_HPP
