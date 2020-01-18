
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
find_package( Boost REQUIRED COMPONENTS filesystem )


function( prtcl_generate_scheme_header _NAME )
  add_custom_command(
    OUTPUT  ${_INC}/prtcl/schemes/${_NAME}.hpp
    COMMAND prtcl-gt-schemes ${_NAME} ${_INC}/prtcl/schemes/${_NAME}.hpp
    DEPENDS prtcl-gt-schemes
    COMMENT "Generating ${_NAME} scheme code."
  )
endfunction( prtcl_generate_scheme_header _NAME  )


prtcl_generate_scheme_header( test )

prtcl_generate_scheme_header( boundary )
prtcl_generate_scheme_header( sesph )
prtcl_generate_scheme_header( symplectic_euler )


add_library( prtcl-schemes INTERFACE )
target_sources(
  prtcl-schemes
  INTERFACE
    ${_INC}/prtcl/schemes/test.hpp

    ${_INC}/prtcl/schemes/boundary.hpp
    ${_INC}/prtcl/schemes/sesph.hpp
    ${_INC}/prtcl/schemes/symplectic_euler.hpp
)
target_include_directories(
  prtcl-schemes
  INTERFACE
    ${_INC}
)
target_link_libraries(
  prtcl-schemes
  INTERFACE prtcl-rt
)


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
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-schemes-sesph

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

  ${_PRJ}/sources/prtcl-schemes-sesph.cpp
)
DE_CXX17_Target( prtcl-schemes-sesph )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-sesph )
DE_LibCXX_Target( prtcl-schemes-sesph )
DE_Add_Our_Include_Directories( prtcl-schemes-sesph include sources )
target_compile_definitions(
  prtcl-schemes-sesph
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-schemes-sesph
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-schemes-sesph
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC Boost::filesystem
  PUBLIC ${CMAKE_DL_LIBS}
)

