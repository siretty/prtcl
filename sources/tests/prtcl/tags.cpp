#include <catch.hpp>

#include <prtcl/tags.hpp>

#include <sstream>

TEST_CASE("prtcl::tag", "[prtcl][tag]") {
  auto tag_to_string = [](auto tag) {
    std::ostringstream s;
    s << tag;
    return s.str();
  };

  REQUIRE("active" == tag_to_string(prtcl::tag::active{}));
  REQUIRE("passive" == tag_to_string(prtcl::tag::passive{}));

  REQUIRE("uniform" == tag_to_string(prtcl::tag::uniform{}));
  REQUIRE("varying" == tag_to_string(prtcl::tag::varying{}));

  REQUIRE("scalar" == tag_to_string(prtcl::tag::scalar{}));
  REQUIRE("vector" == tag_to_string(prtcl::tag::vector{}));

  REQUIRE("host" == tag_to_string(prtcl::tag::host{}));
  REQUIRE("sycl" == tag_to_string(prtcl::tag::sycl{}));
}
