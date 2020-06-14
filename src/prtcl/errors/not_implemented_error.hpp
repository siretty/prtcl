#ifndef PRTCL_SRC_PRTCL_ERRORS_NOT_IMPLEMENTED_ERROR_HPP
#define PRTCL_SRC_PRTCL_ERRORS_NOT_IMPLEMENTED_ERROR_HPP

#include <stdexcept>

class NotImplementedError : public std::exception {
  char const *what() const noexcept final { return "not implemented"; }
};

#endif // PRTCL_SRC_PRTCL_ERRORS_NOT_IMPLEMENTED_ERROR_HPP
