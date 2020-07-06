#include "../util/archive.hpp"
#include "uniform_field.hpp"

#include <gtest/gtest.h>

using namespace prtcl;

TEST(UniformField, SaveLoad) {
  std::string data;

  {
    std::ostringstream os;
    NativeBinaryArchiveWriter ar{os};

    auto field = MakeUniformField<int64_t>();
    (*field.Span<int64_t>()) = 1234;
    field.Save(ar);

    data = os.str();
  }

  {
    std::istringstream is{data};
    NativeBinaryArchiveReader ar{is};

    auto field = MakeUniformField<int64_t>();
    field.Load(ar);

    ASSERT_EQ(1234, *field.Span<int64_t>());
  }
}