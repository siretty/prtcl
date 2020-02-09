#include <catch2/catch.hpp>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

#include <prtcl/rt/cli/command_line_interface.hpp>

#include <iostream>

TEST_CASE("prtcl/rt/cli/command_line_interface.hpp", "[prtcl][rt]") {
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

  prtcl::rt::command_line_interface cli{static_cast<int>(argv.size()),
                                        argv.data()};

  std::vector<std::string> positional;
  std::copy(
      cli.positional().begin(), cli.positional().end(),
      std::back_inserter(positional));
  CHECK(2 == positional.size());
  CHECK(positional[0] == "positional-1");
  CHECK(positional[1] == "positional-2");

  CHECK(cli.has_name_only_argument("long-flag-1"));
  CHECK(cli.has_name_only_argument("nested.long-flag-1"));
  CHECK(cli.has_name_only_argument("long-flag-2"));
  CHECK(cli.has_name_only_argument("nested.long-flag-2"));

  {
    CHECK(cli.has_name_value_argument("long-sarg-1"));

    std::vector<std::string> values;
    cli.values_with_name<std::string>("long-sarg-1", std::back_inserter(values));
    CHECK(1 == values.size());
    CHECK("value-1" == values[0]);
  }

  {
    CHECK(cli.has_name_value_argument("nested.long-sarg-1"));

    std::vector<std::string> values;
    cli.values_with_name<std::string>("nested.long-sarg-1", std::back_inserter(values));
    CHECK(1 == values.size());
    CHECK("value-1" == values[0]);
  }

  {
    CHECK(cli.has_name_value_argument("long-sarg-2"));
    
    std::vector<std::string> values;
    cli.values_with_name<std::string>("long-sarg-2", std::back_inserter(values));
    CHECK(1 == values.size());
    CHECK("value-2" == values[0]);
  }

  {
    CHECK(cli.has_name_value_argument("nested.long-sarg-2"));

    std::vector<std::string> values;
    cli.values_with_name<std::string>("nested.long-sarg-2", std::back_inserter(values));
    CHECK(1 == values.size());
    CHECK("value-2" == values[0]);
  }
}
