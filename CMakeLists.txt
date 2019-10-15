
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

find_package( Threads )
find_package( OpenMP )

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

set( triSYCL_PREFIX_DIR "${CMAKE_CURRENT_BINARY_DIR}/subprojects/triSYCL" )
set( triSYCL_INCLUDE_DIR "${triSYCL_PREFIX_DIR}/include" )
ExternalProject_Add(
  triSYCL
  PREFIX     ${CMAKE_CURRENT_BINARY_DIR}/subprojects/triSYCL.build
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/subprojects/triSYCL
  CMAKE_ARGS
    -DBUILD_TESTING=FALSE
    -DCMAKE_INSTALL_PREFIX=${triSYCL_PREFIX_DIR}
)

set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )

DE_Add_Catch2_Main( test_main )
DE_LibCXX_Target( test_main )

DE_Add_Catch2_Test(
  prtcl-tests test_main
  ${_SRC}/tests/prtcl/dummy.cpp
  # linear data / buffer / access
  ${_SRC}/tests/prtcl/data/host/host_linear_data.cpp
  ${_SRC}/tests/prtcl/data/host/host_linear_access.cpp
  ${_SRC}/tests/prtcl/data/sycl/sycl_linear_buffer.cpp
  ${_SRC}/tests/prtcl/data/sycl/sycl_linear_access.cpp
  # scalar data / buffer / access
  ${_SRC}/tests/prtcl/data/host/vector_of_scalars.cpp
  ${_SRC}/tests/prtcl/data/sycl/vector_of_scalars.cpp
  # array data / buffer / access
  ${_SRC}/tests/prtcl/data/host/array_of_vectors.cpp
  ${_SRC}/tests/prtcl/data/sycl/array_of_vectors.cpp
  ${_SRC}/tests/prtcl/data/host/vector_of_arrays.cpp
  ${_SRC}/tests/prtcl/data/sycl/vector_of_arrays.cpp
  # group data
  ${_SRC}/tests/prtcl/data/group_data.cpp
  # expr terminal
  #${_SRC}/tests/prtcl/expr/terminal/uniform_varying_scalar_vector.cpp
  #${_SRC}/tests/prtcl/expr/grammar/scalar.cpp
  #${_SRC}/tests/prtcl/expr/grammar/vector.cpp
  # transforms
  #${_SRC}/tests/prtcl/expr/transform/for_each_used_field.cpp
  #${_SRC}/tests/prtcl/expr/transform/bind_field_data.cpp
  # equation
  ${_SRC}/tests/prtcl/equation.cpp
  # tag types
  ${_SRC}/tests/prtcl/tags.cpp
  # expression types
  ${_SRC}/tests/prtcl/expression/field_data.cpp
  # schemes
  ${_SRC}/tests/prtcl/scheme/basic_scheme.cpp
  # math traits
  ${_SRC}/tests/prtcl/math/traits/host_math_traits.cpp
  ${_SRC}/tests/prtcl/math/traits/sycl_math_traits.cpp
  # math kernel
  ${_SRC}/tests/prtcl/math/kernel/cubic_spline_kernel.cpp
)

DE_CXX17_Target( prtcl-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-tests )
DE_LibCXX_Target( prtcl-tests )
DE_Add_Our_Include_Directories( prtcl-tests include sources )
target_include_directories(
  prtcl-tests
  PUBLIC
    ${triSYCL_INCLUDE_DIR}
)
target_link_libraries(
  prtcl-tests
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
)

add_executable(
  prtcl-tests-data-sycl-device-accessor
  ${_SRC}/tests/prtcl/data/sycl/sycl_linear_access_device.cpp
)
DE_CXX17_Target( prtcl-tests-data-sycl-device-accessor )
DE_Common_Diagnostics_CXX_Target( prtcl-tests-data-sycl-device-accessor )
DE_LibCXX_Target( prtcl-tests-data-sycl-device-accessor )
DE_Add_Our_Include_Directories( prtcl-tests-data-sycl-device-accessor include sources )
target_include_directories(
  prtcl-tests-data-sycl-device-accessor
  PUBLIC
    ${triSYCL_INCLUDE_DIR}
)
target_link_libraries(
  prtcl-tests-data-sycl-device-accessor
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
)

