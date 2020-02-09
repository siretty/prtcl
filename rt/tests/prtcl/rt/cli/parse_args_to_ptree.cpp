#include <catch2/catch.hpp>

#include <string>
#include <vector>

#include <prtcl/rt/cli/parse_args_to_ptree.hpp>

#include <iostream>

TEST_CASE("prtcl/rt/cli/parse_args_to_ptree.hpp", "[prtcl][rt]") {
  std::string prog = "test-prog";
  std::vector<std::string> argv_strings = {
      // positional args
      "positional-1",
      "positional-2",
      // long flags
      "--long-flag-1",
      "--long-flag-2",
      "--nested.long-flag-1",
      "--nested.long-flag-2",
      // long single argumets
      "--long-sarg-1=value-1",
      "--long-sarg-2=value-2",
      "--nested.long-sarg-1=value-1",
      "--nested.long-sarg-2=value-2",
  };

  std::vector<char *> argv = {prog.data()};
  for (auto &arg_string : argv_strings)
    argv.push_back(arg_string.data());

  auto args = prtcl::rt::parse_args_to_ptree(
      static_cast<int>(argv.size()), argv.data());

  {
    auto arg = args.get_optional<std::string>("name_only.long-flag-1");
    CHECK(arg.has_value());
    CHECK(arg.value() == "");
  }

  {
    auto arg = args.get_optional<std::string>("name_only.nested.long-flag-1");
    CHECK(arg.has_value());
    CHECK(arg.value() == "");
  }

  {
    auto arg = args.get_optional<std::string>("name_only.long-flag-2");
    CHECK(arg.has_value());
    CHECK(arg.value() == "");
  }

  {
    auto arg = args.get_optional<std::string>("name_only.nested.long-flag-2");
    CHECK(arg.has_value());
    CHECK(arg.value() == "");
  }

  { // check single argument
    auto sarg = args.get_optional<std::string>("name_value.long-sarg-1");
    CHECK(sarg.has_value());
    CHECK(sarg.value() == "value-1");
  }

  { // check single argument
    auto sarg = args.get_optional<std::string>("name_value.nested.long-sarg-1");
    CHECK(sarg.has_value());
    CHECK(sarg.value() == "value-1");
  }

  { // check single argument
    auto sarg = args.get_optional<std::string>("name_value.long-sarg-2");
    CHECK(sarg.has_value());
    CHECK(sarg.value() == "value-2");
  }

  { // check single argument
    auto sarg = args.get_optional<std::string>("name_value.nested.long-sarg-2");
    CHECK(sarg.has_value());
    CHECK(sarg.value() == "value-2");
  }
}
