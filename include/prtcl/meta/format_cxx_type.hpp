#pragma once

#include <prtcl/meta/overload.hpp>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/type_index.hpp>
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

namespace prtcl::meta {

template <typename E, typename S>
void display_cxx_type(E const &, S &out, char indent_marker = '|') {
  auto generate_indent = [indent_marker](auto it, size_t indent) {
    if (indent_marker == ' ')
      std::fill_n(it, 4 * indent, ' ');
    else
      for (size_t i = 0; i < indent; ++i) {
        *(it++) = '|';
        *(it++) = ' ';
        *(it++) = ' ';
        *(it++) = ' ';
      }
  };

  std::string type;

  try {
    type = boost::typeindex::type_id<E>().pretty_name();
  } catch (...) {
    out << boost::typeindex::type_id<E>().name()
        << " [type demangling failed]\n";
    return;
  }

  { // make expr_kind readable
    namespace qi = boost::spirit::qi;

    std::ostringstream ss;

    auto capture_expr_kind = [&ss](unsigned long k) {
      ss << boost::yap::op_string(static_cast<boost::yap::expr_kind>(k));
    };
    auto capture = [&ss](auto v) { ss << v; };

    qi::parse(type.begin(), type.end(),
              *((qi::lit("(boost::yap::expr_kind)") >>
                 qi::ulong_[capture_expr_kind] >> &qi::char_(',')) |
                qi::char_[capture]));

    type = ss.str();
  }

  auto putter = std::ostreambuf_iterator<char>{out};
  size_t indent = 0;

  for (size_t pos = 0; pos < type.size(); ++pos) {
    char cur = type[pos];

    switch (cur) {
    case '<': {
      ++indent;
      *(putter++) = cur;
      *(putter++) = '\n';
      generate_indent(putter, indent);
    } break;

    case '>': {
      --indent;
      *(putter++) = '\n';
      generate_indent(putter, indent);
      *(putter++) = cur;
    } break;

    case ',': {
      *(putter++) = cur;
      *(putter++) = '\n';
      generate_indent(putter, indent);
      while (pos < type.size() && type[pos] != ' ')
        ++pos;
    } break;

    default: {
      *(putter++) = cur;
    } break;
    }
  }
  *(putter++) = '\n';
}

template <typename E>
std::string format_cxx_type(E const &e, char indent_marker = '|') {
  std::ostringstream out;
  display_cxx_type(e, out, indent_marker);
  return out.str();
}

} // namespace prtcl::meta

using prtcl::meta::display_cxx_type;
using prtcl::meta::format_cxx_type;
