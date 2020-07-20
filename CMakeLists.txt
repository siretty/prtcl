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


include(prtcl_add_library)
include(prtcl_add_executable)

include(DECommonDiagnosticFlags)
include(DELibCXXFlags)
include(DECXX17)
include(DEAddOurIncludeDirectories)


add_subdirectory(external/googletest)
add_subdirectory(external/sol2)

set(UNGRD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(UNGRD_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/ungrd)


set(S ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)
