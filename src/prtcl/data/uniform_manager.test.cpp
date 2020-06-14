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
      auto &x = manager->AddFieldImpl<float, 2, 3>("x");
      ASSERT_EQ(manager->GetFieldCount(), 1);
      ASSERT_EQ(x.GetType(), (TensorType{ComponentType::kFloat32, {2, 3}}));
      ASSERT_EQ(x.GetSize(), 1);
    }
    {
      auto *x = manager->TryGetFieldImpl<float, 2, 3>("x");
      ASSERT_NE(x, nullptr);
      ASSERT_EQ(x->GetSize(), 1);
    }

    {
      auto x = manager->TryGetFieldImpl<float, 2, 3>("x")->GetAccessImpl();
      ASSERT_EQ(x.GetSize(), 1);
    }
    {
      auto x = manager->AddFieldImpl<float, 2, 3>("x").GetAccessImpl();
      ASSERT_EQ(x.GetSize(), 1);
    }

    {
      TensorType const y_ttype{ComponentType::kBoolean, {3}};
      auto &y = manager->AddField("y", y_ttype);
      ASSERT_EQ(manager->GetFieldCount(), 2);
      ASSERT_EQ(y.GetType(), y_ttype);
      ASSERT_EQ(y.GetSize(), 1);
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
