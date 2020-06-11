#include <gtest/gtest.h>

#include "group.hpp"

#include <iterator>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

TEST(DataTests, CheckGroup) {
  using namespace prtcl;

  {
    Group group{"name", "type"};

    ASSERT_EQ(group.GetGroupName(), "name");
    ASSERT_EQ(group.GetGroupType(), "type");

    ASSERT_EQ(group.GetSize(), 0);

    {
      ASSERT_TRUE(std::empty(group.GetTags()));

      ASSERT_FALSE(group.HasTag("tag_one"));
      group.AddTag("tag_one");
      ASSERT_TRUE(group.HasTag("tag_one"));

      group.AddTag("tag_two");

      std::vector<std::string> tags;
      boost::copy(group.GetTags(), std::back_inserter(tags));
      boost::sort(tags);
      ASSERT_EQ(tags[0], "tag_one");
      ASSERT_EQ(tags[1], "tag_two");

      group.RemoveTag("tag_one");
      ASSERT_FALSE(group.HasTag("tag_one"));
      ASSERT_TRUE(group.HasTag("tag_two"));

      group.RemoveTag("tag_two");
      ASSERT_FALSE(group.HasTag("tag_one"));
    }

    ASSERT_EQ(group.GetUniform().GetFieldCount(), 0);
    ASSERT_EQ(std::as_const(group).GetUniform().GetFieldCount(), 0);

    ASSERT_EQ(group.GetVarying().GetFieldCount(), 0);
    ASSERT_EQ(std::as_const(group).GetVarying().GetFieldCount(), 0);

    auto &uf_a = group.AddUniformField<float, 2, 3>("a");
    ASSERT_EQ(group.GetUniform().GetFieldCount(), 1);
    ASSERT_EQ(uf_a.GetSize(), 1);

    ASSERT_THROW(
        { (void)(group.AddVaryingField<float, 2, 3>("a")); },
        FieldOfDifferentKindAlreadyExistsError);

    auto &vf_b = group.AddVaryingField<float, 3, 2>("b");
    ASSERT_EQ(group.GetVarying().GetFieldCount(), 1);

    group.Resize(10);
    ASSERT_EQ(group.GetSize(), 10);
    ASSERT_EQ(group.GetVarying().GetItemCount(), 10);
    ASSERT_EQ(vf_b.GetSize(), 10);

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);
    ASSERT_FALSE(group.IsDirty());

    {
      auto indices = group.CreateItems(5);
      ASSERT_EQ(indices.size(), 5);
      ASSERT_EQ(group.GetSize(), 15);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 15);
      ASSERT_EQ(vf_b.GetSize(), 15);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);

    {
      std::array<size_t, 10> const indices{1, 4, 6, 7, 11, 3, 5, 13, 0, 8};
      group.DestroyItems(indices);
      ASSERT_EQ(group.GetSize(), 5);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 5);
      ASSERT_EQ(vf_b.GetSize(), 5);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);

    {
      auto b = vf_b.GetAccess();
      b.SetComponent(0, {0, 0}, 1);
      b.SetComponent(1, {0, 0}, 2);
      b.SetComponent(2, {0, 0}, 3);
      b.SetComponent(3, {0, 0}, 4);
      b.SetComponent(4, {0, 0}, 5);

      std::array<size_t, 5> const perm{3, 4, 1, 2, 0};
      group.Permute(perm);
      ASSERT_EQ(group.GetSize(), 5);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 5);
      ASSERT_EQ(vf_b.GetSize(), 5);

      ASSERT_FLOAT_EQ(b.GetComponent(0, {0, 0}), 4);
      ASSERT_FLOAT_EQ(b.GetComponent(1, {0, 0}), 5);
      ASSERT_FLOAT_EQ(b.GetComponent(2, {0, 0}), 2);
      ASSERT_FLOAT_EQ(b.GetComponent(3, {0, 0}), 3);
      ASSERT_FLOAT_EQ(b.GetComponent(4, {0, 0}), 1);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);
  }
}
