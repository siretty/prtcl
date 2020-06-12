
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost 1.70 REQUIRED COMPONENTS container)

find_package(Lua 5.3 REQUIRED)

find_package(GTest REQUIRED)

prtcl_add_library_simple(
    prtcl TYPE STATIC

    POSITION_INDEPENDENT_CODE

    PHYSICAL_COMPONENTS
    prtcl/config

    prtcl/cxx # c++ compatibility

    prtcl/constpow
    prtcl/morton_order
    prtcl/scheduler
    prtcl/virtual_clock
    prtcl/is_valid_identifier

    prtcl/data/component_type
    prtcl/data/shape
    prtcl/data/tensor_type

    prtcl/data/collection_of_mutable_tensors
    prtcl/data/collection_of_mutable_tensors_impl
    prtcl/data/vector_of_tensors

    prtcl/data/uniform_manager
    prtcl/data/varying_manager

    prtcl/data/model
    prtcl/data/group

    prtcl/errors/field_does_not_exist
    prtcl/errors/field_of_different_type_already_exists_error
    prtcl/errors/field_of_different_kind_already_exists_error
    prtcl/errors/invalid_identifier_error

    prtcl/log/level
    prtcl/log/logger
    prtcl/log/ostream_logger
    prtcl/log

    prtcl/math/eigen
    prtcl/math/impl
    prtcl/math

    EXTRA_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/prtcl/cxx/span.inc

    LINK_LIBRARIES
    PUBLIC Eigen3::Eigen Boost::container
)

prtcl_add_library_simple(
    prtcl_lua TYPE SHARED

    PHYSICAL_COMPONENTS
    prtcl-lua/entry_point

    LINK_LIBRARIES
    PUBLIC prtcl ${LUA_LIBRARIES} sol2

    INCLUDE_DIRECTORIES
    PUBLIC ${LUA_INCLUDE_DIR}
)
target_compile_definitions(
    prtcl_lua
    PUBLIC SOL_ALL_SAFETIES_ON=1
)

add_executable(
    prtcl-lua
    prtcl-lua/main
)
target_link_libraries(
    prtcl-lua
    PUBLIC prtcl_lua
)
set_target_properties(
    prtcl-lua

    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_library(
    prtcl-idecollector STATIC
    ${S}/prtcl/config.cpp
    )
target_link_libraries(
    prtcl-idecollector
    prtcl prtcl_lua
)

set_target_properties(
    prtcl-idecollector

    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
