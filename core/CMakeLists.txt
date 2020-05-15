
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
    ${_INC_PC}/narray.hpp
)
target_include_directories(
  prtcl-core
  INTERFACE 
    ${_INC}
)
target_link_libraries(
  prtcl-core
  INTERFACE
    ${_TARGET_BOOST_HEADERS}
)


DE_Add_Catch2_Test(
  prtcl-core-tests test_main

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

  ${_PRJ}/tests/prtcl/core/narray.cpp
)
DE_CXX17_Target( prtcl-core-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-core-tests )
DE_LibCXX_Target( prtcl-core-tests )
DE_Add_Our_Include_Directories( prtcl-core-tests include sources )
target_compile_definitions(
  prtcl-core-tests
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-core-tests
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-core-tests
  PUBLIC prtcl-core
  PUBLIC ${CMAKE_DL_LIBS}
)
