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
#include <prtcl/gt/printer/prtcl.hpp>

#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

int prtcl_generate_cxx_openmp(
    std::string const &prog, std::vector<std::string> const &args_) {
  if (args_.size() < 3) {
    std::cerr << "usage: (" << prog
              << ") {input file} {output file} {scheme name} {namespace}*"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << R"MSG(
  {input file} : Specifies the path to the input file in .prtcl format.
                 If the argument is '-' the program reads it's input from
                 stdin and acts as a filter.

  {output file} : Specifies the path to the output file.
                  If the argument is '-' the program reads it's input from
                  stdin and acts as a filter.

  {scheme name} : Specifies the name of the scheme to generate code for.

  {namespace} : The namespace the scheme type is part of.
  )MSG";
    std::cerr << std::endl;
    return 1;
  }

  std::string input;
  { // type-erased input stream
    std::istream *input_stream = &std::cin;

    // raii for an actual input file
    std::unique_ptr<std::ifstream> input_file;
    // open the input file if requested
    if (args_[0] != "-") {
      input_file = std::make_unique<std::ifstream>(args_[0], std::ios::in);
      input_stream = input_file.get();
    }

    // disable white-space skipping
    input_stream->unsetf(std::ios::skipws);

    // read the whole input from the stream
    input = std::string{std::istreambuf_iterator<char>(*input_stream),
                        std::istreambuf_iterator<char>()};
  }

  std::unique_ptr<std::ofstream> output_file;
  std::ostream *output_stream = &std::cout;

  // open the output file if requested
  if (args_[1] != "-") {
    output_file = std::make_unique<std::ofstream>(args_[1], std::ios::out);
    output_stream = output_file.get();
  }

  std::string scheme_name = args_[2];

  // split namespaces of the args vector
  auto namespaces = boost::make_iterator_range(args_);
  namespaces.advance_begin(3);

  // setup the printer
  prtcl::gt::printer::cxx_openmp printer{*output_stream, args_[2], namespaces};
  // prtcl::gt::printer::prtcl printer{*output_stream};

  using iterator_type = typename decltype(input)::const_iterator;

  // create forward iterators into the input stream
  iterator_type first = input.begin();
  iterator_type last = input.end();

  // create the error handler
  boost::spirit::x3::error_handler<iterator_type> error_handler{first, last,
                                                                std::cerr};

  // parse the source file
  auto result = prtcl::gt::parse_prtcl_source(first, last, error_handler);

  bool printed = false;

  if (result.abstract_syntax_tree.has_value()) {
    try {
      for (auto &statement : result.abstract_syntax_tree->statements) {
        if (auto *scheme = std::get_if<prtcl::gt::ast::scheme>(&statement)) {
          if (scheme->name == scheme_name) {
            // multiple schemes of the same name in the same file are invalid
            // TODO: package this in a validation function
            if (printed)
              throw prtcl::gt::ast_printer_error{};

            printed = true;
            printer(*scheme);
          }
        }
      }

      // scheme was not found in the input file
      if (not printed)
        throw prtcl::gt::ast_printer_error{};

    } catch (char const *error_) {
      std::cerr << "error: printing failed:" << std::endl;
      std::cerr << error_ << std::endl;
      return 1;
    }
  }

  if (not result.remaining_input.empty()) {
    std::cerr << "error: the input was only parsed partially" << std::endl;

#if defined(PRTCL_GT_PRINT_REMAINING_INPUT)
    std::cerr << "error: remaining input follows" << std::endl;
    std::ostreambuf_iterator<
        typename decltype(std::cerr)::char_type,
        typename decltype(std::cerr)::traits_type>
        output_it{std::cerr};
    std::copy(
        result.remaining_input.begin(), result.remaining_input.end(),
        output_it);
#endif // defined(PRTCL_GT_PRINT_REMAINING_INPUT)

    return 2;
  }

  if (not result.abstract_syntax_tree.has_value())
    return 1;

  return 0;
}
