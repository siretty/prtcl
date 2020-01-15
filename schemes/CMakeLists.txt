
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-schemes
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )


find_package( Threads REQUIRED )
find_package( Eigen3 REQUIRED )
find_package( OpenMP REQUIRED )


DE_Add_Catch2_Test(
  prtcl-schemes-tests test_main

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

  ${_PRJ}/tests/prtcl/schemes/test_counting_particles.cpp
)
DE_CXX17_Target( prtcl-schemes-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-tests )
DE_LibCXX_Target( prtcl-schemes-tests )
DE_Add_Our_Include_Directories( prtcl-schemes-tests include sources )
target_compile_definitions(
  prtcl-schemes-tests
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-schemes-tests
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-schemes-tests
  PUBLIC prtcl-rt
  PUBLIC Threads::Threads
  PUBLIC Eigen3::Eigen
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)

