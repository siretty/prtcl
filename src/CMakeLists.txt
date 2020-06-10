
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost 1.70 REQUIRED COMPONENTS container)

add_library(libprtcl-develtarget STATIC
    ${S}/prtcl/log.cpp)

set(libprtcl_components)
list(
    APPEND libprtcl_components

    prtcl/config

    # c++ compatibility
    prtcl/cxx

    prtcl/constpow
    prtcl/morton_order
    prtcl/scheduler
    prtcl/virtual_clock

    prtcl/group
    prtcl/model

    prtcl/tensors
    prtcl/vector_of_tensors

    prtcl/data/varying_field_manager

    prtcl/errors/invalid_identifier_error
    prtcl/errors/field_exists_error

    prtcl/log/level
    prtcl/log/logger
    prtcl/log/ostream_logger
    prtcl/log

    prtcl/math/eigen
    prtcl/math/impl
    prtcl/math
)

set(libprtcl_sources "${libprtcl_components}")
list(TRANSFORM libprtcl_sources PREPEND "${S}/")
list(TRANSFORM libprtcl_sources APPEND ".cpp")

set(libprtcl_headers "${libprtcl_components}")
list(TRANSFORM libprtcl_headers PREPEND "${I}/")
list(TRANSFORM libprtcl_headers APPEND ".hpp")

prtcl_add_library(
    libprtcl STATIC

    PRIVATE_SOURCES
    ${libprtcl_sources}

    PUBLIC_HEADERS
    ${libprtcl_headers}
)
set_target_properties(libprtcl PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )
target_include_directories(libprtcl PUBLIC ${I} PRIVATE ${S})
target_link_libraries(libprtcl Eigen3::Eigen Boost::container)
