
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-core
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )

set( _INC ${_PRJ}/include )
set( _INC_PC ${_INC}/prtcl/core )


add_library( prtcl-core INTERFACE )
target_sources(
  prtcl-core
  INTERFACE
    ${_INC_PC}/invalid_enumerator_error.hpp
    ${_INC_PC}/enumerator_name.hpp
    ${_INC_PC}/shape.hpp
    ${_INC_PC}/remove_cvref.hpp
    ${_INC_PC}/macros.hpp
    ${_INC_PC}/constpow.hpp
    ${_INC_PC}/morton_order.hpp
)
target_include_directories(
  prtcl-core
  INTERFACE 
    ${_INC}
)
target_link_libraries(
  prtcl-core
  INTERFACE
    prtcl-dep-boost-container
)
