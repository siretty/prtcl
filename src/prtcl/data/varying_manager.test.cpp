#include <gtest/gtest.h>

#include "varying_manager.hpp"

#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

TEST(DataTests, CheckVaryingManager) {
  using namespace prtcl;

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
      auto &x = manager->AddFieldImpl<float, 2, 3>("x");
      ASSERT_EQ(manager->GetFieldCount(), 1);
      ASSERT_EQ(x.GetType(), (TensorType{ComponentType::kFloat32, {2, 3}}));
      ASSERT_EQ(x.GetSize(), 5);
    }
    {
      auto *x = manager->TryGetFieldImpl<float, 2, 3>("x");
      ASSERT_NE(x, nullptr);
      ASSERT_EQ(x->GetSize(), 5);
    }

    manager->ResizeItems(10);
    ASSERT_TRUE(manager->IsDirty());

    {
      auto x = manager->TryGetFieldImpl<float, 2, 3>("x")->GetAccessImpl();
      ASSERT_EQ(x.GetSize(), 10);
    }
    {
      auto x = manager->AddFieldImpl<float, 2, 3>("x").GetAccessImpl();
      ASSERT_EQ(x.GetSize(), 10);
    }

    {
      auto &y = manager->AddFieldImpl<bool, 3, 1>("y");
      ASSERT_EQ(manager->GetFieldCount(), 2);
      ASSERT_EQ(y.GetType(), (TensorType{ComponentType::kBoolean, {3, 1}}));
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
