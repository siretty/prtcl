#include <gtest/gtest.h>

#include "uniform_manager.hpp"

#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

using namespace prtcl;

TEST(DataTests, CheckUniformManager) {
  {
    auto manager = std::make_shared<UniformManager>();

    ASSERT_EQ(manager->GetFieldCount(), 0);

    {
      auto x = manager->AddFieldImpl<float, 2, 3>("x");
      ASSERT_EQ(manager->GetFieldCount(), 1);
      ASSERT_TRUE(x);
    }
    {
      auto x = manager->FieldSpan<float, 2, 3>("x");
      ASSERT_TRUE(x);
    }

    {
      auto x = manager->FieldSpan<float, 2, 3>("x");
      ASSERT_TRUE(x);
    }
    {
      auto x = manager->FieldSpan<float, 2, 3>("x");
      ASSERT_TRUE(x);
    }

    {
      TensorType const y_ttype{ComponentType::kBoolean, {3}};
      auto y = manager->AddField("y", y_ttype);
      ASSERT_EQ(manager->GetFieldCount(), 2);
      ASSERT_TRUE((y.Is<bool, 3>()));
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

TEST(UniformManager, SaveLoadTest) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    UniformManager uniform;

    auto b = uniform.AddFieldImpl<bool, 1, 2>("bx1x2");
    (*b)(0, 1) = true;

    auto s = uniform.AddFieldImpl<int32_t, 3, 1>("s32x3x1");
    (*s)(2, 0) = 12;

    auto d = uniform.AddFieldImpl<double, 2, 3>("f64x2x3");
    (*d)(1, 1) = 12.0;

    ASSERT_EQ(3, uniform.GetFieldCount());

    uniform.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    UniformManager uniform;
    uniform.Load(ar);

    ASSERT_EQ(3, uniform.GetFieldCount());

    auto b = uniform.FieldSpan<bool, 1, 2>("bx1x2");
    ASSERT_EQ(true, (*b)(0, 1));

    auto s = uniform.FieldSpan<int32_t, 3, 1>("s32x3x1");
    ASSERT_EQ(12, (*s)(2, 0));

    auto d = uniform.FieldSpan<double, 2, 3>("f64x2x3");
    ASSERT_EQ(12.0, (*d)(1, 1));
  }
}
