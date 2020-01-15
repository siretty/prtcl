
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-rt
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )


find_package( Eigen3 REQUIRED )


add_library( prtcl-rt INTERFACE )
target_sources(
  prtcl-rt
  INTERFACE
    ${_INC}/prtcl/rt/common.hpp

    ${_INC}/prtcl/rt/nd_data_base.hpp
    ${_INC}/prtcl/rt/grouped_uniform_grid.hpp

    ${_INC}/prtcl/rt/basic_type_policy.hpp
    ${_INC}/prtcl/rt/eigen_math_policy.hpp
    ${_INC}/prtcl/rt/vector_data_policy.hpp
)
target_include_directories(
  prtcl-rt
  INTERFACE 
    ${_INC}
)
target_link_libraries(
  prtcl-rt
  INTERFACE prtcl-core
)


DE_Add_Catch2_Test(
  prtcl-rt-tests test_main

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

  ${_PRJ}/tests/prtcl/rt/eigen_math_policy.cpp
  ${_PRJ}/tests/prtcl/rt/vector_data_policy.cpp
)
DE_CXX17_Target( prtcl-rt-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-rt-tests )
DE_LibCXX_Target( prtcl-rt-tests )
DE_Add_Our_Include_Directories( prtcl-rt-tests include sources )
target_compile_definitions(
  prtcl-rt-tests
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-rt-tests
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-rt-tests
  PUBLIC prtcl-core
  PUBLIC Eigen3::Eigen
  PUBLIC ${CMAKE_DL_LIBS}
)

