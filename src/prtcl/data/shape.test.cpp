#include <gtest/gtest.h>

#include "shape.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>

using namespace prtcl;

TEST(DataTests, CheckShape) {
  {
    std::initializer_list<size_t> input = {1, 2, 3, 4};

    Shape shape{input};
    ASSERT_FALSE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 4);
    ASSERT_EQ(shape.GetExtents().size(), 4);
    ASSERT_EQ(shape.GetExtents()[0], 1);
    ASSERT_EQ(shape.GetExtents()[1], 2);
    ASSERT_EQ(shape.GetExtents()[2], 3);
    ASSERT_EQ(shape.GetExtents()[3], 4);
  }

  {
    std::array<size_t, 4> input = {1, 2, 3, 4};

    Shape shape{input};
    ASSERT_FALSE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 4);
    ASSERT_EQ(shape.GetExtents().size(), 4);
    ASSERT_EQ(shape.GetExtents()[0], 1);
    ASSERT_EQ(shape.GetExtents()[1], 2);
    ASSERT_EQ(shape.GetExtents()[2], 3);
    ASSERT_EQ(shape.GetExtents()[3], 4);
  }

  {
    Shape shape;
    ASSERT_TRUE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 0);
    ASSERT_EQ(shape.GetExtents().size(), 0);
    ASSERT_EQ(shape.ToString(), "[]");
    ASSERT_EQ(Shape::FromString("[]"), shape);
  }

  {
    Shape shape{1};
    ASSERT_FALSE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 1);
    ASSERT_EQ(shape.GetExtents().size(), 1);
    ASSERT_EQ(shape.GetExtents()[0], 1);
    ASSERT_EQ(shape.ToString(), "[1]");
    ASSERT_EQ(Shape::FromString("[1]"), shape);
  }

  {
    Shape shape{3, 2, 1};
    ASSERT_FALSE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 3);
    ASSERT_EQ(shape.GetExtents().size(), 3);
    ASSERT_EQ(shape.GetExtents()[0], 3);
    ASSERT_EQ(shape.GetExtents()[1], 2);
    ASSERT_EQ(shape.GetExtents()[2], 1);
    ASSERT_EQ(shape.ToString(), "[3, 2, 1]");
    ASSERT_EQ(Shape::FromString("[3, 2, 1]"), shape);
  }

  {
    Shape shape{3, 0, 1};
    ASSERT_TRUE(shape.IsEmpty());
    ASSERT_EQ(shape.GetRank(), 3);
    ASSERT_EQ(shape.GetExtents().size(), 3);
    ASSERT_EQ(shape.GetExtents()[0], 3);
    ASSERT_EQ(shape.GetExtents()[1], 0);
    ASSERT_EQ(shape.GetExtents()[2], 1);
    ASSERT_EQ(shape.ToString(), "[3, 0, 1]");
    ASSERT_EQ(Shape::FromString("[3, 0, 1]"), shape);
  }
}

TEST(Shape, SaveLoadTest) {
  {
    std::string data;
    {
      std::ostringstream os;
      NativeBinaryArchiveWriter ar{os};

      Shape shape{3, 2, 1};
      shape.Save(ar);

      data = os.str();
    }

    {
      std::istringstream is{data};
      NativeBinaryArchiveReader ar{is};

      Shape shape;
      shape.Load(ar);

      ASSERT_EQ(3, shape.GetRank());
      ASSERT_EQ(3, shape[0]);
      ASSERT_EQ(2, shape[1]);
      ASSERT_EQ(1, shape[2]);
    }
  }
}
