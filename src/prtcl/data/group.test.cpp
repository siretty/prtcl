#include <gtest/gtest.h>

#include "group.hpp"
#include "model.hpp"

#include <iterator>
#include <vector>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>

using namespace prtcl;

TEST(DataTests, CheckGroup) {
  Model model;

  {
    Group group{model, "name", "type"};

    ASSERT_EQ(group.GetGroupName(), "name");
    ASSERT_EQ(group.GetGroupType(), "type");

    ASSERT_EQ(group.GetItemCount(), 0);

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

    auto uf_a = group.AddUniformFieldImpl<float, 2, 3>("a");
    ASSERT_EQ(group.GetUniform().GetFieldCount(), 1);
    ASSERT_TRUE(uf_a);

    ASSERT_THROW(
        { (void)(group.AddVaryingFieldImpl<float, 2, 3>("a")); },
        FieldOfDifferentKindAlreadyExistsError);

    auto vf_b = group.AddVaryingFieldImpl<float, 3, 2>("b");
    ASSERT_EQ(group.GetVarying().GetFieldCount(), 1);

    group.Resize(10);
    ASSERT_EQ(group.GetItemCount(), 10);
    ASSERT_EQ(group.GetVarying().GetItemCount(), 10);

    vf_b = group.GetVarying().FieldSpan<float, 3, 2>("b");
    ASSERT_EQ(vf_b.GetSize(), 10);

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);
    ASSERT_FALSE(group.IsDirty());

    {
      auto indices = group.CreateItems(5);
      ASSERT_EQ(indices.size(), 5);
      ASSERT_EQ(group.GetItemCount(), 15);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 15);

      vf_b = group.GetVarying().FieldSpan<float, 3, 2>("b");
      ASSERT_EQ(vf_b.GetSize(), 15);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);

    {
      std::array<size_t, 10> const indices{1, 4, 6, 7, 11, 3, 5, 13, 0, 8};
      group.DestroyItems(indices);
      ASSERT_EQ(group.GetItemCount(), 5);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 5);

      vf_b = group.GetVarying().FieldSpan<float, 3, 2>("b");
      ASSERT_EQ(vf_b.GetSize(), 5);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);

    {
      auto b = vf_b;
      b[0](0, 0) = 1;
      b[1](0, 0) = 2;
      b[2](0, 0) = 3;
      b[3](0, 0) = 4;
      b[4](0, 0) = 5;

      std::array<size_t, 5> const perm{3, 4, 1, 2, 0};
      group.Permute(perm);
      ASSERT_EQ(group.GetItemCount(), 5);
      ASSERT_EQ(group.GetVarying().GetItemCount(), 5);
      ASSERT_EQ(vf_b.GetSize(), 5);

      ASSERT_FLOAT_EQ(b[0](0, 0), 4);
      ASSERT_FLOAT_EQ(b[1](0, 0), 5);
      ASSERT_FLOAT_EQ(b[2](0, 0), 2);
      ASSERT_FLOAT_EQ(b[3](0, 0), 3);
      ASSERT_FLOAT_EQ(b[4](0, 0), 1);
    }

    ASSERT_TRUE(group.IsDirty());
    group.SetDirty(false);
  }
}

TEST(Group, SaveLoadTest) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    Model model;
    auto &group = model.AddGroup("g", "t");

    group.AddVaryingFieldImpl<float, 1, 2>("a");
    group.AddUniformFieldImpl<bool, 1, 2>("b");
    group.Resize(102);

    group.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    Model model;
    auto &group = model.AddGroup("h", "s");

    group.Load(ar);

    ASSERT_EQ(102, group.GetItemCount());
    ASSERT_TRUE((group.GetVarying().FieldSpan<float, 1, 2>("a")));
    ASSERT_TRUE((group.GetUniform().FieldSpan<bool, 1, 2>("b")));
  }
}
