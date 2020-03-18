
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-gt
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )


prtcl_add_library(
  prtcl-gt
  TYPE INTERFACE
  INTERFACE_SOURCES
    ${_INC}/prtcl/gt/common.hpp

    ${_INC}/prtcl/gt/ast.hpp
    ${_INC}/prtcl/gt/bits/ast_define.hpp
    ${_INC}/prtcl/gt/bits/ast_fusion.hpp

    ${_INC}/prtcl/gt/parser.hpp
    ${_INC}/prtcl/gt/bits/parser_declare.hpp
    ${_INC}/prtcl/gt/bits/parser_define.hpp
    ${_INC}/prtcl/gt/bits/parser_spirit.hpp

    # ...
)
target_include_directories( prtcl-gt INTERFACE ${_INC} )
target_link_libraries( prtcl-gt
  INTERFACE prtcl-core
  INTERFACE prtcl-dep-boost-headers
)


add_executable(
  prtcl-gt-schemes

  ${_SRC}/prtcl/gt/schemes.cpp
)
DE_CXX17_Target( prtcl-gt-schemes )
DE_Common_Diagnostics_CXX_Target( prtcl-gt-schemes )
DE_LibCXX_Target( prtcl-gt-schemes )
DE_Add_Our_Include_Directories( prtcl-gt-schemes include sources )
target_compile_definitions(
  prtcl-gt-schemes
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-gt-schemes
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-gt-schemes
  PUBLIC prtcl-gt
  PUBLIC ${CMAKE_DL_LIBS}
)

