#pragma once

#include <prtcl/tag/kind.hpp>
#include <prtcl/tag/type.hpp>

#include <string>

#include <boost/hana.hpp>

namespace prtcl::data {

template <typename Value>
  class field_map {
    using value_type = Value;

    template <typename KT, typename TT> void insert(std::string name, value_type const & value) {
      //
    }

  };

} //
