
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(Boost 1.70 REQUIRED COMPONENTS headers container)

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

find_package(Lua 5.3 REQUIRED)

find_package(GTest REQUIRED)


include(prtcl_generate_scheme)


prtcl_generate_scheme(density)
prtcl_generate_scheme(gravity)
prtcl_generate_scheme(viscosity)
prtcl_generate_scheme(correction)

prtcl_generate_scheme(symplectic_euler)

prtcl_generate_scheme(sesph)
prtcl_generate_scheme(iisph)

prtcl_generate_scheme(aiast12)
prtcl_generate_scheme(aat13)
prtcl_generate_scheme(he14)
prtcl_generate_scheme(pt16)
prtcl_generate_scheme(wkbb18)

prtcl_generate_scheme(horas)


# prtcl (currently) must be a _shared_ library since the linker removes
# the registration of schemes when linked statically. It can be used as
# a static library but then the schemes must be registered explicitly.
prtcl_add_library_simple(
    prtcl TYPE SHARED

    POSITION_INDEPENDENT_CODE

    PHYSICAL_COMPONENTS
    prtcl/config

    prtcl/cxx # c++ compatibility
    prtcl/cxx/map
    prtcl/cxx/set
    prtcl/cxx/span

    prtcl/data/component_type
    prtcl/data/shape
    prtcl/data/tensor_type

    prtcl/data/uniform_manager
    prtcl/data/uniform_field
    prtcl/data/varying_manager
    prtcl/data/varying_field

    prtcl/data/model
    prtcl/data/group

    prtcl/errors/field_does_not_exist
    prtcl/errors/field_of_different_type_already_exists_error
    prtcl/errors/field_of_different_kind_already_exists_error
    prtcl/errors/invalid_identifier_error
    prtcl/errors/invalid_shape_error
    prtcl/errors/not_implemented_error

    prtcl/geometry/triangle_mesh
    prtcl/geometry/sample_surface
    prtcl/geometry/sample_volume
    prtcl/geometry/pinhole_camera

    prtcl/log/level
    prtcl/log/logger
    prtcl/log/ostream_logger
    prtcl/log

    prtcl/math/common
    prtcl/math/constants
    prtcl/math/impl/eigen
    prtcl/math/impl
    prtcl/math/kernel
    prtcl/math/kernel/cubic_spline_kernel
    prtcl/math/kernel/kernel_traits
    prtcl/math/kernel/kernel_facade
    prtcl/math

    prtcl/schemes/scheme_base

    prtcl/schemes/density
    prtcl/schemes/gravity
    prtcl/schemes/viscosity
    prtcl/schemes/correction

    prtcl/schemes/symplectic_euler

    prtcl/schemes/sesph
    prtcl/schemes/iisph

    prtcl/schemes/aiast12
    prtcl/schemes/aat13
    prtcl/schemes/he14
    prtcl/schemes/pt16
    prtcl/schemes/wkbb18

    prtcl/schemes/horas

    prtcl/solver/cg_openmp

    prtcl/util/constpow
    prtcl/util/morton_order
    prtcl/util/is_valid_identifier

    prtcl/util/scheduler
    prtcl/util/virtual_clock

    prtcl/util/neighborhood
    prtcl/util/grouped_uniform_grid

    prtcl/util/hcp_lattice_source
    prtcl/util/integral_grid

    prtcl/util/archive
    prtcl/util/save_vtk

    prtcl/util/sphere_tracer

    EXTRA_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/prtcl/cxx/span.inc

    LINK_LIBRARIES PUBLIC
    ungrd
    Boost::headers Boost::container
    Eigen3::Eigen
    OpenMP::OpenMP_CXX
    Threads::Threads

    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)


prtcl_add_library_simple(
    prtcl_lua TYPE STATIC

    PHYSICAL_COMPONENTS
    prtcl-lua/entry_point
    prtcl-lua/module_data
    prtcl-lua/module_geometry
    prtcl-lua/module_math
    prtcl-lua/module_schemes
    prtcl-lua/module_util

    LINK_LIBRARIES PUBLIC
    prtcl
    ${LUA_LIBRARIES}
    sol2

    INCLUDE_DIRECTORIES
    PUBLIC ${LUA_INCLUDE_DIR}

    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
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
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
