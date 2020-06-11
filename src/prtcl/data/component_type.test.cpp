#include <gtest/gtest.h>

#include "component_type.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>

#include <cstdint>

TEST(DataTests, CheckComponentType) {
  using namespace prtcl;

  {
    ComponentType const ctype;
    ASSERT_TRUE(not ctype.IsValid());
    ASSERT_EQ(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "INVALID");
    ASSERT_EQ(ComponentType::FromString("INVALID"), ctype);
    ASSERT_EQ(ComponentType::FromString(""), ctype);
    ASSERT_EQ(ComponentType::FromString("abcsalk93asdi"), ctype);
  }

  {
    auto const ctype = ComponentType::kBoolean;
    ASSERT_TRUE(ctype.IsValid());
    ASSERT_NE(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "b");
    ASSERT_EQ(ComponentType::FromString("b"), ctype);
  }

  {
    auto const ctype = ComponentType::kSInt32;
    ASSERT_TRUE(ctype.IsValid());
    ASSERT_NE(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "s32");
    ASSERT_EQ(ComponentType::FromString("s32"), ctype);
  }

  {
    auto const ctype = ComponentType::kSInt64;
    ASSERT_TRUE(ctype.IsValid());
    ASSERT_NE(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "s64");
    ASSERT_EQ(ComponentType::FromString("s64"), ctype);
  }

  {
    auto const ctype = ComponentType::kFloat32;
    ASSERT_TRUE(ctype.IsValid());
    ASSERT_NE(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "f32");
    ASSERT_EQ(ComponentType::FromString("f32"), ctype);
  }

  {
    auto const ctype = ComponentType::kFloat64;
    ASSERT_TRUE(ctype.IsValid());
    ASSERT_NE(ctype, ComponentType::kInvalid);
    ASSERT_EQ(ctype.ToStringView(), "f64");
    ASSERT_EQ(ComponentType::FromString("f64"), ctype);
  }

  {
    ASSERT_EQ(MakeComponentType<bool>(), ComponentType::kBoolean);
    ASSERT_EQ(MakeComponentType<int32_t>(), ComponentType::kSInt32);
    ASSERT_EQ(MakeComponentType<int64_t>(), ComponentType::kSInt64);
    ASSERT_EQ(MakeComponentType<float>(), ComponentType::kFloat32);
    ASSERT_EQ(MakeComponentType<double>(), ComponentType::kFloat64);
    ASSERT_EQ(MakeComponentType<std::string>(), ComponentType::kInvalid);
  }
}
