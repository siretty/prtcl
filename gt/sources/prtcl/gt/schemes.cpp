#include <prtcl/gt/parse_prtcl_source.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/spirit/include/support_istream_iterator.hpp>

int prtcl_generate_cxx_openmp(
    std::string const &, std::vector<std::string> const &);

int main(int argc_, char **argv_) {
  std::string prog = argv_[0];
  std::vector<std::string> args;
  for (int i = 1; i < argc_; ++i)
    args.push_back(argv_[i]);

  return prtcl_generate_cxx_openmp(prog, args);
}

#include <prtcl/gt/printer/cxx_openmp.hpp>

#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

int prtcl_generate_cxx_openmp(
    std::string const &prog, std::vector<std::string> const &args_) {
  std::unique_ptr<std::ifstream> input_file;
  std::istream *input = &std::cin;
  std::unique_ptr<std::ofstream> output_file;
  std::ostream *output = &std::cout;

  if (args_.size() < 3) {
    std::cerr << "usage: (" << prog
              << ") {input file} {scheme name} {namespace}+" << std::endl;
    std::cerr << std::endl;
    std::cerr << R"MSG(
  {input file} : Specifies the path to the input file in .prtcl format.
                If the argument is '-' the program reads it's input from
                stdin and acts as a filter.

  {scheme name} : Specifies the name of the scheme, for C++/OpenMP this
                  is used as the name of type encapsulating all procedures
                  as public member functions.

  {namespace} : The namespace the scheme type is part of.
  )MSG";
    std::cerr << std::endl;
    return 1;
  }

  // open the input file if requested
  if (args_[0] != "-") {
    input_file = std::make_unique<std::ifstream>(args_[0], std::ios::in);
    input = input_file.get();
  }

  // open the output file if requested
  if (args_[1] != "-") {
    output_file = std::make_unique<std::ofstream>(args_[1], std::ios::out);
    output = output_file.get();
  }

  // disable white-space skipping
  input->unsetf(std::ios::skipws);

  // split namespaces of the args vector
  auto namespaces = boost::make_iterator_range(args_);
  namespaces.advance_begin(3);

  // setup the printer
  prtcl::gt::printer::cxx_openmp printer{*output, args_[2], namespaces};

  // create forward iterators into the input stream
  auto first = boost::spirit::istream_iterator{*input};
  auto last = decltype(first){};

  // parse the source file
  auto result = prtcl::gt::parse_prtcl_source(first, last);

  if (result.abstract_syntax_tree) {
    try {
    // printer(result.abstract_syntax_tree.value());
    printer(result.abstract_syntax_tree.value());
    } catch (char const *error_) {
      std::cerr << "error: printing failed:" << std::endl;
      std::cerr << error_ << std::endl;
      return 1;
    }
  }

  if (not result.remaining_input.empty()) {
    std::cerr << "error: the input was only parsed partially" << std::endl;
    std::cerr << "error: remaining input follows" << std::endl;
    std::ostreambuf_iterator<
        typename decltype(std::cerr)::char_type,
        typename decltype(std::cerr)::traits_type>
        output_it{std::cerr};
    std::copy(
        result.remaining_input.begin(), result.remaining_input.end(),
        output_it);
    return 2;
  }

  if (not result.abstract_syntax_tree)
    return 1;

  return 0;
}
/*

#include <prtcl/gt/ast.hpp>
//#include <prtcl/gt/schemes_registry.hpp>

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

*/
