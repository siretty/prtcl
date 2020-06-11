#include <gtest/gtest.h>

#include "vector_of_tensors.hpp"

#include <array>

TEST(DataTests, CheckVectorOfTensors) {
  using namespace prtcl;

  {
    VectorOfTensors<float, 3, 2> tensors;

    ASSERT_EQ(tensors.GetType(), (TensorType{ComponentType::kFloat32, {3, 2}}));
    ASSERT_EQ(tensors.GetSize(), 0);

    tensors.Resize(5);

    ASSERT_EQ(tensors.GetSize(), 5);

    std::array<size_t, 2> cidx = {2, 1};

    {
      auto access = tensors.GetAccess();

      access.SetComponent(0, cidx, 1);
      ASSERT_FLOAT_EQ(access.GetComponent(0, cidx), 1);

      access.SetComponent(0, {2, 1}, 2);
      ASSERT_FLOAT_EQ(access.GetComponent(0, {2, 1}), 2);

      access.SetComponentVariant(1, cidx, {3.0f});
      ASSERT_FLOAT_EQ(std::get<float>(access.GetComponentVariant(1, cidx)), 3);

      access.SetComponentVariant(1, {2, 1}, {4.0f});
      ASSERT_FLOAT_EQ(
          std::get<float>(access.GetComponentVariant(1, {2, 1})), 4);

      ASSERT_FLOAT_EQ(access.GetComponent(0, cidx), 2);
      ASSERT_FLOAT_EQ(access.GetComponent(0, {2, 1}), 2);
      ASSERT_FLOAT_EQ(
          std::get<float>(access.GetComponentVariant(0, {2, 1})), 2);

      access.SetComponent(2, cidx, 3);
      access.SetComponent(3, cidx, 4);
      access.SetComponent(4, cidx, 5);
    }

    {
      auto access = tensors.GetAccess();

      access.SetComponent(0, cidx, 1);
      access.SetComponent(1, cidx, 2);
      access.SetComponent(2, cidx, 3);
      access.SetComponent(3, cidx, 4);
      access.SetComponent(4, cidx, 5);

      std::array<size_t, 5> permutation = {2, 4, 3, 0, 1};
      tensors.Permute(permutation);

      ASSERT_FLOAT_EQ(access.GetComponent(0, cidx), 3);
      ASSERT_FLOAT_EQ(access.GetComponent(1, cidx), 5);
      ASSERT_FLOAT_EQ(access.GetComponent(2, cidx), 4);
      ASSERT_FLOAT_EQ(access.GetComponent(3, cidx), 1);
      ASSERT_FLOAT_EQ(access.GetComponent(4, cidx), 2);
    }

    // TODO: cover the remaining methods
  }
}
