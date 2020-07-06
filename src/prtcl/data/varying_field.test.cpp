#include "../util/archive.hpp"
#include "varying_field.hpp"

#include <sstream>

#include <gtest/gtest.h>

using namespace prtcl;

TEST(VaryingField, SaveLoad) {
    std::string data;

    {
      std::ostringstream os;
      NativeBinaryArchiveWriter ar{os};

      auto field = MakeVaryingField<bool>();
      field.Save(ar);

      data = os.str();
    }

    {
      std::istringstream is{data};
      NativeBinaryArchiveReader ar{is};

      auto field = MakeVaryingField<bool>();
      field.Load(ar);

      ASSERT_EQ(0, field.GetSize());
    }

    {
      std::ostringstream os;
      NativeBinaryArchiveWriter ar{os};

      auto field = MakeVaryingField<int64_t, 3, 2>();
      field.Resize(3);
      {
        auto span = field.Span<int64_t, 3, 2>();
        span[0](0, 0) = 123;
        span[1](1, 1) = 456;
        span[2](2, 0) = 789;
      }
      field.Save(ar);

      data = os.str();
    }

    {
      std::istringstream is{data};
      NativeBinaryArchiveReader ar{is};

      auto field = MakeVaryingField<int64_t, 3, 2>();
      field.Load(ar);

      ASSERT_EQ(3, field.GetSize());

      {
        auto span = field.Span<int64_t, 3, 2>();
        ASSERT_EQ(123, span[0](0, 0));
        ASSERT_EQ(456, span[1](1, 1));
        ASSERT_EQ(789, span[2](2, 0));
      }
    }
}