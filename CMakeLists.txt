
cmake_minimum_required(
  VERSION 3.5
  FATAL_ERROR
)

project(
  prtcl
  VERSION 0.1
  LANGUAGES C CXX
)

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

find_package( Threads REQUIRED )
find_package( OpenMP REQUIRED )
find_package( Eigen3 REQUIRED )

include( DECommonDiagnosticFlags )
include( DELibCXXFlags )
include( DECatch2 )
include( DECXX17 )
include( DEAddOurIncludeDirectories )

message( "Common C++ Diagnostic Flags: ${DE_COMMON_DIAGNOSTICS_FLAGS_CXX}" )
message( "LibC++ CXX_FLAGS: ${DE_LibCXX_CXX_FLAGS}" )
message( "LibC++ LINK_FLAGS: ${DE_LibCXX_LINK_FLAGS}" )

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/catch2 )

include( ExternalProject )

set( tracy_INLCUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/subprojects/tracy" )

set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )

DE_Add_Catch2_Main( test_main )
DE_LibCXX_Target( test_main )
target_compile_definitions( test_main
  PUBLIC
    CATCH_CONFIG_ENABLE_BENCHMARKING
)

DE_Add_Catch2_Test(
  prtcl-tests test_main

  # allow tests to be profiled with tracy
  ${tracy_INLCUDE_DIR}/TracyClient.cpp

  # the dummy test
  ${_SRC}/tests/prtcl/dummy.cpp

  ${_SRC}/tests/prtcl/meta/get.cpp

  ${_SRC}/tests/prtcl/data/tensors.cpp
  ${_SRC}/tests/prtcl/data/openmp/tensors.cpp

  ${_SRC}/tests/prtcl/data/uniforms.cpp
  ${_SRC}/tests/prtcl/data/openmp/uniforms.cpp

  ${_SRC}/tests/prtcl/data/varyings.cpp
  ${_SRC}/tests/prtcl/data/openmp/varyings.cpp

  ${_SRC}/tests/prtcl/data/group.cpp
  ${_SRC}/tests/prtcl/data/openmp/group.cpp
  
  ${_SRC}/tests/prtcl/data/scheme.cpp
  ${_SRC}/tests/prtcl/data/openmp/scheme.cpp

  ${_SRC}/tests/prtcl/expr/transform/field_subscript_transform.cpp
  ${_SRC}/tests/prtcl/expr/field_value_transform.cpp

  ${_SRC}/tests/prtcl/scheme/compiler.cpp
  ${_SRC}/tests/prtcl/scheme/openmp/compiler.cpp
)
DE_CXX17_Target( prtcl-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-tests )
DE_LibCXX_Target( prtcl-tests )
DE_Add_Our_Include_Directories( prtcl-tests include sources )
target_compile_definitions(
  prtcl-tests
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-tests
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-tests
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC Eigen3::Eigen
  PUBLIC ${CMAKE_DL_LIBS}
)

