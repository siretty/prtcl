#include <gtest/gtest.h>

#include "is_valid_identifier.hpp"

TEST(DataTests, CheckIsValidIdentifier) {
  using namespace prtcl;

  {
    // positive examples
    ASSERT_TRUE(IsValidIdentifier("x"));
    ASSERT_TRUE(IsValidIdentifier("V"));
    ASSERT_TRUE(IsValidIdentifier("rho0"));
    ASSERT_TRUE(IsValidIdentifier("a0_and_b0"));
    ASSERT_TRUE(IsValidIdentifier("joining_and_trailing_underscore_"));

    // negative examples
    ASSERT_FALSE(IsValidIdentifier("_leading_underscore"));
    ASSERT_FALSE(IsValidIdentifier("full.stop"));
    ASSERT_FALSE(IsValidIdentifier("exclamation mark!"));
    ASSERT_FALSE(IsValidIdentifier("two words"));
    ASSERT_FALSE(IsValidIdentifier("2x"));
    ASSERT_FALSE(IsValidIdentifier("4V"));
    ASSERT_FALSE(IsValidIdentifier("6rho0"));
  }
}
