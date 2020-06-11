
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost 1.70 REQUIRED COMPONENTS container)

prtcl_add_library_simple(
    prtcl STATIC

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

    prtcl/data/group
    prtcl/data/model

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
    PUBLIC
    Eigen3::Eigen
    Boost::container
)

add_library(
    prtcl-idecollector STATIC
    ${S}/prtcl/config.cpp
    )

target_link_libraries(
    prtcl-idecollector
    prtcl
)

set_target_properties(
    prtcl-idecollector

    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
