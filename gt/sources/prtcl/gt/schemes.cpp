#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/schemes_registry.hpp>

#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

int main(int argc_, char **argv_) {
  std::string prog = argv_[0];
  std::vector<std::string> args;
  for (int argi = 1; argi < argc_; ++argi)
    args.emplace_back(argv_[argi]);

  auto &registry = prtcl::gt::get_schemes_registry();

  if (0 == args.size()) {
    for (auto &scheme : registry.schemes())
      std::cout << scheme.name() << '\n';
    return 0;
  } else if (2 < args.size()) {
    std::cerr << "usage: " << prog << " [SCHEME_NAME [OUTPUT_FILE_NAME]]"
              << '\n';
    std::cerr << "error: invalid number of commandline arguments" << '\n';
    return 1;
  }

  std::string scheme_name;
  if (1 <= args.size())
    scheme_name = args[0];

  std::string output_filename = "-";
  if (2 <= args.size())
    output_filename = args[1];

  std::ostream *output = &std::cout;
  if (output_filename != "-")
    output = new std::fstream{output_filename,
                              std::fstream::out | std::fstream::trunc};

  prtcl::gt::ast::cpp_openmp_printer{*output}(
      &registry.get_scheme(scheme_name));

  if (output != &std::cout)
    delete output;
}
