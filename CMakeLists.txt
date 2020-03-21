cmake_minimum_required( VERSION 3.5 FATAL_ERROR )

project(
  prtcl
  VERSION 0.1
  LANGUAGES C CXX
)

# generate compilation database
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# setup the dependency prefix
list( APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/deps/prefix" )

# allow loading our own modules
list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )


# setup the main tracy directory
set( _TRACY_DIR ${CMAKE_SOURCE_DIR}/deps/sources/tracy )
set( _TRACY_CPP ${CMAKE_SOURCE_DIR}/rt/sources/prtcl/rt/log/trace.cpp )


find_package( Catch2 REQUIRED )


include( prtcl_build_boost )

include( prtcl_add_library )
include( prtcl_add_executable )

include( DECommonDiagnosticFlags )
include( DELibCXXFlags )
include( DECatch2 )
include( DECXX17 )
include( DEAddOurIncludeDirectories )


prtcl_build_boost()


DE_Add_Catch2_Main( test_main )
DE_CXX17_Target( test_main )
DE_LibCXX_Target( test_main )
target_compile_definitions( test_main
  PUBLIC
    CATCH_CONFIG_ENABLE_BENCHMARKING
)


add_subdirectory( core )
add_subdirectory( gt )
add_subdirectory( rt )
add_subdirectory( schemes )

