
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-gt
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )


DE_Add_Catch2_Test(
  prtcl-gt-tests test_main

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

  ${_PRJ}/tests/prtcl/gt/field_kind.cpp
  ${_PRJ}/tests/prtcl/gt/field_type.cpp
  ${_PRJ}/tests/prtcl/gt/field.cpp

  ${_PRJ}/tests/prtcl/gt/function.cpp

  ${_PRJ}/tests/prtcl/gt/constant.cpp

  ${_PRJ}/tests/prtcl/gt/field_requirements.cpp

  ${_PRJ}/tests/prtcl/gt/ast.cpp

  ${_PRJ}/tests/prtcl/gt/dsl/procedure.cpp
)
DE_CXX17_Target( prtcl-gt-tests )
DE_Common_Diagnostics_CXX_Target( prtcl-gt-tests )
DE_LibCXX_Target( prtcl-gt-tests )
DE_Add_Our_Include_Directories( prtcl-gt-tests include sources )
target_compile_definitions(
  prtcl-gt-tests
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-gt-tests
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-gt-tests
  PUBLIC prtcl-core
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-gt-schemes-symplectic_euler

  ${_SRC}/prtcl/gt/schemes/symplectic_euler.cpp
)
DE_CXX17_Target( prtcl-gt-schemes-symplectic_euler )
DE_Common_Diagnostics_CXX_Target( prtcl-gt-schemes-symplectic_euler )
DE_LibCXX_Target( prtcl-gt-schemes-symplectic_euler )
DE_Add_Our_Include_Directories( prtcl-gt-schemes-symplectic_euler include sources )
target_compile_definitions(
  prtcl-gt-schemes-symplectic_euler
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-gt-schemes-symplectic_euler
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-gt-schemes-symplectic_euler
  PUBLIC prtcl-core
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-gt-schemes-test

  ${_SRC}/prtcl/gt/schemes/test.cpp
)
DE_CXX17_Target( prtcl-gt-schemes-test )
DE_Common_Diagnostics_CXX_Target( prtcl-gt-schemes-test )
DE_LibCXX_Target( prtcl-gt-schemes-test )
DE_Add_Our_Include_Directories( prtcl-gt-schemes-test include sources )
target_compile_definitions(
  prtcl-gt-schemes-test
  PUBLIC
    TRACY_ENABLE
    CATCH_CONFIG_ENABLE_BENCHMARKING
)
target_include_directories(
  prtcl-gt-schemes-test
  PUBLIC
    ${tracy_INLCUDE_DIR}
)
target_link_libraries(
  prtcl-gt-schemes-test
  PUBLIC prtcl-core
  PUBLIC ${CMAKE_DL_LIBS}
)

