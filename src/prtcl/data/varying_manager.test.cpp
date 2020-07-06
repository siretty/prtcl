#include <gtest/gtest.h>

#include "varying_manager.hpp"

#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

using namespace prtcl;

TEST(DataTests, CheckVaryingManager) {
  {
    auto manager = std::make_shared<VaryingManager>();
    ASSERT_EQ(manager->GetFieldCount(), 0);
    ASSERT_EQ(manager->GetItemCount(), 0);

    manager->ResizeItems(5);
    ASSERT_TRUE(manager->IsDirty());

    manager->SetDirty(false);
    ASSERT_FALSE(manager->IsDirty());

    ASSERT_EQ(manager->GetItemCount(), 5);

    {
      auto x = manager->AddFieldImpl<float, 2, 3>("x");
      ASSERT_EQ(manager->GetFieldCount(), 1);
      ASSERT_EQ(x.GetSize(), 5);
    }
    {
      auto x = manager->FieldSpan<float, 2, 3>("x");
      ASSERT_EQ(x.GetSize(), 5);
    }

    manager->ResizeItems(10);
    ASSERT_TRUE(manager->IsDirty());

    {
      auto x = manager->FieldSpan<float, 2, 3>("x");
      ASSERT_EQ(x.GetSize(), 10);
    }
    {
      auto x = manager->AddFieldImpl<float, 2, 3>("x");
      ASSERT_EQ(x.GetSize(), 10);
    }

    {
      auto y = manager->AddFieldImpl<bool, 3, 1>("y");
      ASSERT_EQ(manager->GetFieldCount(), 2);
      ASSERT_EQ(y.GetSize(), 10);
    }

    {
      ASSERT_EQ(manager->GetItemCount(), 10);
      auto indices = manager->CreateItems(5);
      ASSERT_EQ(indices.size(), 5);
      ASSERT_EQ(indices[0], 10);
      ASSERT_EQ(indices[4], 14);
      ASSERT_EQ(manager->GetItemCount(), 15);
    }

    {
      ASSERT_EQ(manager->GetItemCount(), 15);
      manager->DestroyItems(std::vector<size_t>{2, 5, 6, 9, 11});
      ASSERT_EQ(manager->GetItemCount(), 10);
    }

    std::vector<std::string> names;
    boost::copy(manager->GetFieldNames(), std::back_inserter(names));
    boost::sort(names);
    ASSERT_EQ(names.size(), 2);
    ASSERT_EQ(names[0], "x");
    ASSERT_EQ(names[1], "y");

    manager->RemoveField("x");
    ASSERT_EQ(manager->GetFieldCount(), 1);

    manager->RemoveField("y");
    ASSERT_EQ(manager->GetFieldCount(), 0);

    ASSERT_NO_THROW({ manager->RemoveField("z"); });
  }
}

TEST(VaryingManager, SaveLoadTest) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    VaryingManager varying;
    varying.ResizeItems(5);

    auto b = varying.AddFieldImpl<bool, 1, 2>("bx1x2");
    b[0](0, 1) = true;
    b[1](0, 1) = false;
    b[2](0, 1) = true;
    b[3](0, 1) = false;
    b[4](0, 1) = true;

    auto s = varying.AddFieldImpl<int32_t, 3, 1>("s32x3x1");
    s[0](2, 0) = 12;
    s[1](2, 0) = 34;
    s[2](2, 0) = 56;
    s[3](2, 0) = 78;
    s[4](2, 0) = 90;

    auto d = varying.AddFieldImpl<double, 2, 3>("f64x2x3");
    d[0](1, 1) = 12.0;
    d[1](1, 1) = 34.0;
    d[2](1, 1) = 56.0;
    d[3](1, 1) = 78.0;
    d[4](1, 1) = 90.0;

    ASSERT_EQ(3, varying.GetFieldCount());
    ASSERT_EQ(5, varying.GetItemCount());

    varying.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    VaryingManager varying;
    varying.Load(ar);

    ASSERT_EQ(3, varying.GetFieldCount());
    ASSERT_EQ(5, varying.GetItemCount());

    auto b = varying.FieldSpan<bool, 1, 2>("bx1x2");
    ASSERT_EQ(true, b[0](0, 1));
    ASSERT_EQ(false, b[1](0, 1));
    ASSERT_EQ(true, b[2](0, 1));
    ASSERT_EQ(false, b[3](0, 1));
    ASSERT_EQ(true, b[4](0, 1));

    auto s = varying.FieldSpan<int32_t, 3, 1>("s32x3x1");
    ASSERT_EQ(12, s[0](2, 0));
    ASSERT_EQ(34, s[1](2, 0));
    ASSERT_EQ(56, s[2](2, 0));
    ASSERT_EQ(78, s[3](2, 0));
    ASSERT_EQ(90, s[4](2, 0));

    auto d = varying.FieldSpan<double, 2, 3>("f64x2x3");
    ASSERT_EQ(12.0, d[0](1, 1));
    ASSERT_EQ(34.0, d[1](1, 1));
    ASSERT_EQ(56.0, d[2](1, 1));
    ASSERT_EQ(78.0, d[3](1, 1));
    ASSERT_EQ(90.0, d[4](1, 1));
  }
}
