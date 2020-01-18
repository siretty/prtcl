
cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl-gt
  VERSION 0.1
  LANGUAGES C CXX
)


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )


add_library( prtcl-gt INTERFACE )
target_sources(
  prtcl-gt
  INTERFACE
    ${_INC}/prtcl/gt/ast/ast_leaf_base.hpp
    ${_INC}/prtcl/gt/ast/ast_nary_base.hpp
    ${_INC}/prtcl/gt/ast/ast_nary_crtp.hpp
    ${_INC}/prtcl/gt/ast/ast_node_base.hpp
    ${_INC}/prtcl/gt/ast.hpp
    ${_INC}/prtcl/gt/common.hpp
    ${_INC}/prtcl/gt/dsl/common.hpp
    ${_INC}/prtcl/gt/dsl/component_index.hpp
    ${_INC}/prtcl/gt/dsl/constant.hpp
    ${_INC}/prtcl/gt/dsl/field.hpp
    ${_INC}/prtcl/gt/dsl/foreach_neighbor.hpp
    ${_INC}/prtcl/gt/dsl/foreach_particle.hpp
    ${_INC}/prtcl/gt/dsl/if_group_type.hpp
    ${_INC}/prtcl/gt/dsl/literal.hpp
    ${_INC}/prtcl/gt/dsl/neighbor_index.hpp
    ${_INC}/prtcl/gt/dsl/operation.hpp
    ${_INC}/prtcl/gt/dsl/particle_index.hpp
    ${_INC}/prtcl/gt/dsl/procedure.hpp
    ${_INC}/prtcl/gt/dsl/reduce.hpp
    ${_INC}/prtcl/gt/dsl.hpp
    ${_INC}/prtcl/gt/dsl_to_ast.hpp
    ${_INC}/prtcl/gt/schemes_registry.hpp
)
target_include_directories(
  prtcl-gt
  INTERFACE
    ${_INC}
)
target_link_libraries(
  prtcl-gt
  INTERFACE prtcl-core
)


DE_Add_Catch2_Test(
  prtcl-gt-tests test_main

  # allow tests to be profiled with tracy
  #${tracy_INLCUDE_DIR}/TracyClient.cpp

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
  PUBLIC prtcl-gt
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-gt-schemes

  ${_SRC}/prtcl/gt/schemes.cpp
  ${_SRC}/prtcl/gt/schemes_registry.cpp

  ${_SRC}/prtcl/gt/schemes/test.cpp

  ${_SRC}/prtcl/gt/schemes/boundary.cpp
  ${_SRC}/prtcl/gt/schemes/sesph.cpp
  ${_SRC}/prtcl/gt/schemes/symplectic_euler.cpp
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

