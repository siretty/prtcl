#include <gtest/gtest.h>

#include "model.hpp"

#include <iterator>
#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

TEST(DataTests, CheckModel) {
  using namespace prtcl;

  {
    Model model;
    ASSERT_EQ(model.GetGroupCount(), 0);
    ASSERT_FALSE(model.IsDirty());

    auto &group_a = model.AddGroup("a", "type");
    ASSERT_EQ(model.GetGroupCount(), 1);
    ASSERT_EQ(group_a.GetItemCount(), 0);

    auto &group_b = model.AddGroup("b", "type");
    ASSERT_EQ(model.GetGroupCount(), 2);
    ASSERT_EQ(group_b.GetItemCount(), 0);

    ASSERT_EQ(&group_a, model.TryGetGroup("a"));

    ASSERT_FALSE(model.IsDirty());

    group_a.CreateItems(5);
    ASSERT_TRUE(model.IsDirty());
    ASSERT_TRUE(group_a.IsDirty());
    ASSERT_FALSE(group_b.IsDirty());

    model.SetDirty(false);
    ASSERT_FALSE(model.IsDirty());
    ASSERT_FALSE(group_a.IsDirty());
    ASSERT_FALSE(group_b.IsDirty());

    {
      std::vector<std::string> names;
      boost::copy(model.GetGroupNames(), std::back_inserter(names));
      boost::sort(names);

      ASSERT_EQ(names.size(), 2);
      ASSERT_EQ(names[0], "a");
      ASSERT_EQ(names[1], "b");
    }

    {
      auto &gf_a = model.AddGlobalField<float, 2, 3>("a");
      ASSERT_EQ(model.GetGlobal().GetFieldCount(), 1);
      ASSERT_EQ(gf_a.GetSize(), 1);

      ASSERT_THROW(
          { (void)(model.AddGlobalField<float, 3, 2>("a")); },
          FieldOfDifferentTypeAlreadyExistsError);
    }

    model.RemoveGlobalField("a");
    ASSERT_EQ(model.GetGlobal().GetFieldCount(), 0);

    model.RemoveGroup("a");
    ASSERT_EQ(model.GetGroupCount(), 1);
  }
}
