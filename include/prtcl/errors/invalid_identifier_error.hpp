#ifndef PRTCL_INVALID_IDENTIFIER_ERROR_HPP
#define PRTCL_INVALID_IDENTIFIER_ERROR_HPP

#include <stdexcept>

namespace prtcl {

class InvalidIdentifierError : public std::exception {
public:
  char const *what() const noexcept override { return "invalid identifier"; }
};

} // namespace prtcl

#endif // PRTCL_INVALID_IDENTIFIER_ERROR_HPP
