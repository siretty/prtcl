#pragma once

#include <boost/property_tree/ptree.hpp>

#include <boost/utility/string_view.hpp>

namespace prtcl::rt {

inline auto parse_args_to_ptree(int argc_, char **argv_) {
  namespace pt = boost::property_tree;

  // create the argument storage
  pt::ptree tree;

  // iterate over each argument
  for (int argi = 1; argi < argc_; ++argi) {
    boost::string_view arg{argv_[argi]};

    if (2 < arg.size() and arg.starts_with("--")) {
      // find long options (starting with --)
      if (auto pos = arg.find_first_of('='); pos != arg.npos) {
        auto key = arg.substr(2, pos - 2), value = arg.substr(pos + 1);
        // long argument (with value)
        tree.add("name_value." + key.to_string(), value.to_string());
      } else {
        // long flag (without value)
        auto key = arg.substr(2);
        tree.add("name_only." + key.to_string(), "");
      }
    } else {
      // positional argument
      tree.add("positional", arg.to_string());
    }
  }

  return tree;
}

} // namespace prtcl::rt
