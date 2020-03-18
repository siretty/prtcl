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

//#include <prtcl/gt/printer/cxx_openmp.hpp>
#include <prtcl/gt/printer/prtcl.hpp>

#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/range/iterator_range.hpp>

//#include <boost/spirit/home/x3.hpp>
//
// using iterator_type = std::string::iterator;
//
// using phrase_context_type =
//    boost::spirit::x3::phrase_parse_context<prtcl::gt::skipper_type>::type;
//
// using context_type = boost::spirit::x3::context<
//    prtcl::gt::parser::position_cache_tag,
//    std::reference_wrapper<prtcl::gt::position_cache_t<iterator_type>>,
//    phrase_context_type>;
//
// namespace prtcl::gt::parser {
//
// BOOST_SPIRIT_INSTANTIATE(prtcl::gt::parser_type, iterator_type,
// context_type);
//
//} // namespace prtcl::gt::parser

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

  // split namespaces of the args vector
  auto namespaces = boost::make_iterator_range(args_);
  namespaces.advance_begin(3);

  for (auto &ns : namespaces) {
    std::cout << "namespace " << ns << std::endl;
  }

  // setup the printer
  // prtcl::gt::printer::cxx_openmp printer{*output, args_[2], namespaces};
  prtcl::gt::printer::prtcl printer{*output_stream};

  using iterator_type = typename decltype(input)::const_iterator;

  // create forward iterators into the input stream
  iterator_type first = input.begin();
  iterator_type last = input.end();

  // create the error handler
  boost::spirit::x3::error_handler<iterator_type> error_handler{first, last,
                                                                std::cerr};

  // parse the source file
  auto result = prtcl::gt::parse_prtcl_source(first, last, error_handler);

  if (result.abstract_syntax_tree.has_value()) {
    try {
      printer(result.abstract_syntax_tree.value());
    } catch (char const *error_) {
      std::cerr << "error: printing failed:" << std::endl;
      std::cerr << error_ << std::endl;
      return 1;
    }
  }

  if (not result.remaining_input.empty()) {
    std::cerr << "error: the input was only parsed partially" << std::endl;
    // std::cerr << "error: remaining input follows" << std::endl;
    // std::ostreambuf_iterator<
    //    typename decltype(std::cerr)::char_type,
    //    typename decltype(std::cerr)::traits_type>
    //    output_it{std::cerr};
    // std::copy(
    //    result.remaining_input.begin(), result.remaining_input.end(),
    //    output_it);
    return 2;
  }

  if (not result.abstract_syntax_tree.has_value())
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
