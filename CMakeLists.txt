cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(
    prtcl
    VERSION 0.1
    LANGUAGES C CXX
)

# generate compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# allow loading our own modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


# setup the main tracy directory
set(_TRACY_DIR ${CMAKE_SOURCE_DIR}/deps/sources/tracy)
set(_TRACY_CPP ${CMAKE_SOURCE_DIR}/rt/sources/prtcl/rt/log/trace.cpp)


find_package(Eigen3 REQUIRED)
message(STATUS "Found Eigen3: ${Eigen3_DIR} (found version \"${Eigen3_VERSION}\")")

find_package(Catch2 REQUIRED)
message(STATUS "Found Catch2: ${Catch2_DIR} (found version \"${Catch2_VERSION}\")")

find_package(Boost REQUIRED)
set(_TARGET_BOOST_HEADERS Boost::headers)
set(_TARGET_BOOST_CONTAINER Boost::container)


include(prtcl_add_library)
include(prtcl_add_executable)

include(DECommonDiagnosticFlags)
include(DELibCXXFlags)
include(DECatch2)
include(DECXX17)
include(DEAddOurIncludeDirectories)


DE_Add_Catch2_Main(test_main)
DE_CXX17_Target(test_main)
DE_LibCXX_Target(test_main)
target_compile_definitions(test_main
    PUBLIC
    CATCH_CONFIG_ENABLE_BENCHMARKING
    )


add_subdirectory(core)
add_subdirectory(rt)
add_subdirectory(schemes)


set(I ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(S ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(tests)
