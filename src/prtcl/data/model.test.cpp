#include <gtest/gtest.h>

#include "model.hpp"

#include <iterator>
#include <string>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

using namespace prtcl;

TEST(DataTests, CheckModel) {
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
      auto gf_a = model.AddGlobalFieldImpl<float, 2, 3>("a");
      ASSERT_EQ(model.GetGlobal().GetFieldCount(), 1);
      ASSERT_TRUE(gf_a);

      ASSERT_THROW(
          { (void)(model.AddGlobalFieldImpl<float, 3, 2>("a")); },
          FieldOfDifferentTypeAlreadyExistsError);
    }

    model.RemoveGlobalField("a");
    ASSERT_EQ(model.GetGlobal().GetFieldCount(), 0);

    model.RemoveGroup("a");
    ASSERT_EQ(model.GetGroupCount(), 1);
  }
}

TEST(Model, SaveLoadTest) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    Model model;
    model.AddGroup("a", "a_type");
    model.AddGroup("b", "b_type");
    model.AddGlobalFieldImpl<bool, 1, 2>("bx1x2");

    model.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    Model model;
    model.Load(ar);

    ASSERT_NE(nullptr, model.TryGetGroup("a"));
    ASSERT_EQ("a_type", model.TryGetGroup("a")->GetGroupType());

    ASSERT_NE(nullptr, model.TryGetGroup("b"));
    ASSERT_EQ("b_type", model.TryGetGroup("b")->GetGroupType());

    ASSERT_EQ(1, model.GetGlobal().GetFieldCount());
    ASSERT_TRUE((model.GetGlobal().FieldSpan<bool, 1, 2>("bx1x2")));
  }
}
