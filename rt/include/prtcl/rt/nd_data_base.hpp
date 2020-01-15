#pragma once

#include "common.hpp"

#include <cstddef>

namespace prtcl::rt {

class nd_data_base {
public:
  virtual ~nd_data_base() {}

public:
  virtual nd_dtype dtype() const = 0;

  virtual nd_shape shape() const = 0;

public:
  virtual size_t size() const = 0;

public:
  virtual void resize(size_t) = 0;
};

} // namespace prtcl::rt
