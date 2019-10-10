#pragma once

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

#include <boost/type_index.hpp>

#include <iostream>

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
