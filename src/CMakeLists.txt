
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost 1.70 REQUIRED COMPONENTS container)

prtcl_add_library_simple(
    libprtcl STATIC

    PHYSICAL_COMPONENTS
    prtcl/config

    prtcl/cxx # c++ compatibility

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

    LINK_LIBRARIES
    Eigen3::Eigen
    Boost::container
)

add_library(
    libprtcl-idecollector STATIC
    ${S}/prtcl/config.cpp
)
