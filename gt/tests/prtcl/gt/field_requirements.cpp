#include <catch2/catch.hpp>

#include <prtcl/gt/field_literals.hpp>
#include <prtcl/gt/field_requirements.hpp>

#include <iostream>

#include <boost/range/algorithm/equal.hpp>

TEST_CASE("prtcl/gt/field_requirements", "[prtcl]") {
  using namespace ::prtcl::gt::field_literals;

  ::prtcl::gt::field_requirements reqs;

  CHECK_NOTHROW(reqs.add_requirement("f"_grs));
  CHECK_NOTHROW(reqs.add_requirement("f"_grv));
  CHECK_NOTHROW(reqs.add_requirement("f"_grm));

  CHECK_THROWS(reqs.add_requirement("f"_urs));
  CHECK_THROWS(reqs.add_requirement("f"_urv));
  CHECK_THROWS(reqs.add_requirement("f"_urm));

  CHECK_THROWS(reqs.add_requirement("f"_vrs));
  CHECK_THROWS(reqs.add_requirement("f"_vrv));
  CHECK_THROWS(reqs.add_requirement("f"_vrm));

  CHECK_THROWS(reqs.add_requirement("a", "f"_grs));
  CHECK_THROWS(reqs.add_requirement("a", "f"_grv));
  CHECK_THROWS(reqs.add_requirement("a", "f"_grm));

  CHECK_NOTHROW(reqs.add_requirement("a", "f"_urs));
  CHECK_NOTHROW(reqs.add_requirement("a", "f"_urv));
  CHECK_NOTHROW(reqs.add_requirement("a", "f"_urm));

  CHECK_NOTHROW(reqs.add_requirement("a", "f"_vrs));
  CHECK_NOTHROW(reqs.add_requirement("a", "f"_vrv));
  CHECK_NOTHROW(reqs.add_requirement("a", "f"_vrm));

  CHECK_NOTHROW(reqs.add_requirement("b", "f"_urs));
  CHECK_NOTHROW(reqs.add_requirement("b", "f"_vrs));

  using field_set = std::set<::prtcl::gt::field>;

  CHECK(boost::range::equal(
      field_set{"f"_urs, "f"_urv, "f"_urm, "f"_vrs, "f"_vrv, "f"_vrm},
      reqs.fields("a")));
  CHECK(boost::range::equal(
      field_set{"f"_urs, "f"_urv, "f"_urm}, reqs.uniform_fields("a")));
  CHECK(boost::range::equal(field_set{"f"_urs}, reqs.uniform_fields("b")));

  CHECK(boost::range::equal(
      field_set{"f"_vrs, "f"_vrv, "f"_vrm}, reqs.varying_fields("a")));
  CHECK(boost::range::equal(field_set{"f"_vrs}, reqs.varying_fields("b")));

  using string_set = std::set<std::string>;

  CHECK_THROWS(reqs.groups("f"_grs));

  CHECK(boost::range::equal(string_set{"a", "b"}, reqs.groups("f"_urs)));
  CHECK(boost::range::equal(string_set{"a", "b"}, reqs.groups("f"_vrs)));

  CHECK(boost::range::equal(string_set{"a"}, reqs.groups("f"_urv)));
  CHECK(boost::range::equal(string_set{"a"}, reqs.groups("f"_vrv)));
}
