#pragma once

#include <prtcl/core/log/logger.hpp>

#include <prtcl/gt/ast.hpp>
#include <prtcl/gt/parser.hpp>

#include <iostream>
#include <optional>
#include <vector>

#include <boost/range/iterator_range.hpp>

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>

namespace prtcl::gt {

template <typename Iterator_> struct parse_prtcl_source_result {
  std::optional<ast::prtcl_file> abstract_syntax_tree;
  boost::iterator_range<Iterator_> remaining_input;
};

template <typename Iterator_>
inline parse_prtcl_source_result<Iterator_> parse_prtcl_source(
    Iterator_ first, Iterator_ last,
    boost::spirit::x3::error_handler<Iterator_> &error_handler) {
  namespace x3 = boost::spirit::x3;

  // create the annotated parser
  auto const the_parser =
      x3::with<x3::error_handler_tag>(std::ref(error_handler))[ //
          parser::prtcl_file                                    //
  ];

  // invoke the parser
  ast::prtcl_file ast;
  bool success = x3::phrase_parse( //
      first, last,                 //
      the_parser,                  //
      parser::white_space,         //
      ast                          //
  );

  // create the result with the remaining input range
  parse_prtcl_source_result<Iterator_> result = {
      {},
      boost::make_iterator_range(first, last),
  };

  // return the ast to the caller if parsing succeeded
  if (success) {
    result.abstract_syntax_tree = std::move(ast);

    core::log::debug(
        "gt", "parse_prtcl_source", "successfully parsed input with ",
        boost::empty(result.remaining_input) ? "no " : "", "remaining input");
  } else {
    core::log::error(
        "gt", "parse_prtcl_source", "failed to parse the input with ",
        boost::empty(result.remaining_input) ? "no " : "", "remaining input");
  }

  return result;
}

} // namespace prtcl::gt
