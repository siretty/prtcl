#pragma once

#include "ast.hpp"
#include "parser.hpp"
#include "prtcl/gt/bits/ast_define.hpp"

#include <optional>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace prtcl::gt {

template <typename Iterator_> struct parse_prtcl_source_result {
  std::optional<ast::prtcl_file> abstract_syntax_tree;
  boost::iterator_range<Iterator_> remaining_input;
};

template <typename Iterator_>
inline parse_prtcl_source_result<Iterator_>
parse_prtcl_source(Iterator_ first, Iterator_ last) {
  namespace x3 = boost::spirit::x3;

  // invoke the parser
  ast::prtcl_file attribute;
  bool success = x3::phrase_parse( //
      first, last,                 //
      parser::prtcl_file,          //
      parser::white_space,         //
      attribute                    //
  );

  // create the result with the remaining input range
  parse_prtcl_source_result<Iterator_> result = {
      {},
      boost::make_iterator_range(first, last),
  };

  // return the ast to the caller if parsing succeeded
  if (success)
    result.abstract_syntax_tree = std::move(attribute);

  return result;
}

} // namespace prtcl::gt
