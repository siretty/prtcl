#include <gtest/gtest.h>

#include "tensor_type.hpp"

using namespace prtcl;

TEST(DataTests, CheckTensorType) {
  {
    TensorType const ttype;
    ASSERT_EQ(ttype.GetComponentType(), ComponentType::kInvalid);
    ASSERT_EQ(ttype.GetShape(), Shape{});
    ASSERT_EQ(ttype.GetComponentCount(), 0);
    ASSERT_FALSE(ttype.IsValid());
    ASSERT_TRUE(ttype.IsEmpty());
  }

  {
    TensorType const ttype{ComponentType::kFloat32};
    ASSERT_EQ(ttype.GetComponentType(), ComponentType::kFloat32);
    ASSERT_EQ(ttype.GetShape(), Shape{});
    ASSERT_EQ(ttype.GetComponentCount(), 0);
    ASSERT_TRUE(ttype.IsValid());
    ASSERT_TRUE(ttype.IsEmpty());
  }

  {
    TensorType const ttype{ComponentType::kInvalid, {1, 2}};
    ASSERT_EQ(ttype.GetComponentType(), ComponentType::kInvalid);
    ASSERT_EQ(ttype.GetShape(), (Shape{1, 2}));
    ASSERT_EQ(ttype.GetComponentCount(), 1 * 2);
    ASSERT_FALSE(ttype.IsValid());
    ASSERT_FALSE(ttype.IsEmpty());
  }

  {
    TensorType const base_ttype{ComponentType::kFloat32, {1, 2}};

    auto const ttype = base_ttype.WithShape({3, 4, 5});
    ASSERT_EQ(ttype.GetComponentType(), base_ttype.GetComponentType());
    ASSERT_EQ(ttype.GetShape(), (Shape{3, 4, 5}));
    ASSERT_EQ(ttype.GetComponentCount(), 3 * 4 * 5);
  }

  {
    TensorType const base_ttype{ComponentType::kFloat32, {1, 2}};

    auto const ttype = base_ttype.WithComponentType(ComponentType::kSInt32);
    ASSERT_EQ(ttype.GetComponentType(), ComponentType::kSInt32);
    ASSERT_EQ(ttype.GetShape(), base_ttype.GetShape());
  }

  {
    TensorType const base_ttype{ComponentType::kFloat32, {1, 2}};
    TensorType const from_ttype{ComponentType::kSInt64, {3, 4, 5}};

    {
      auto const ttype = base_ttype.WithComponentTypeOf(from_ttype);
      ASSERT_EQ(ttype.GetComponentType(), from_ttype.GetComponentType());
      ASSERT_EQ(ttype.GetShape(), base_ttype.GetShape());
      ASSERT_EQ(ttype.GetComponentCount(), 1 * 2);
    }

    {
      auto const ttype = base_ttype.WithShapeOf(from_ttype);
      ASSERT_EQ(ttype.GetComponentType(), base_ttype.GetComponentType());
      ASSERT_EQ(ttype.GetShape(), from_ttype.GetShape());
      ASSERT_EQ(ttype.GetComponentCount(), 3 * 4 * 5);
    }
  }
}

TEST(TensorType, SaveLoadTest) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    TensorType const ttype{ComponentType::kFloat32, {1, 2}};
    ttype.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    TensorType ttype;
    ttype.Load(ar);

    ASSERT_EQ(ComponentType::kFloat32, ttype.GetComponentType());
    ASSERT_EQ((Shape{1, 2}), ttype.GetShape());
  }
}
