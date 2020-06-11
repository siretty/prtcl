#include <gtest/gtest.h>

#include "uniform_manager.hpp"

#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

TEST(DataTests, CheckUniformManager) {
  using namespace prtcl;

  {
    auto manager = std::make_shared<UniformManager>();

    ASSERT_EQ(manager->GetFieldCount(), 0);

    {
      auto &x = manager->AddField<float, 2, 3>("x");
      ASSERT_EQ(manager->GetFieldCount(), 1);
      ASSERT_EQ(x.GetType(), (TensorType{ComponentType::kFloat32, {2, 3}}));
      ASSERT_EQ(x.GetSize(), 1);
    }
    {
      auto *x = manager->TryGetField<float, 2, 3>("x");
      ASSERT_NE(x, nullptr);
      ASSERT_EQ(x->GetSize(), 1);
    }

    {
      auto x = manager->TryGetField<float, 2, 3>("x")->GetAccess();
      ASSERT_EQ(x.GetSize(), 1);
    }
    {
      auto x = manager->AddField<float, 2, 3>("x").GetAccess();
      ASSERT_EQ(x.GetSize(), 1);
    }

    {
      auto &y = manager->AddField<bool, 3, 1>("y");
      ASSERT_EQ(manager->GetFieldCount(), 2);
      ASSERT_EQ(y.GetType(), (TensorType{ComponentType::kBoolean, {3, 1}}));
      ASSERT_EQ(y.GetSize(), 1);
    }

    std::vector<std::string> names;
    boost::copy(manager->GetNames(), std::back_inserter(names));
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
