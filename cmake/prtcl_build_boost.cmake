
function( prtcl_build_boost )
  set( BOOST_DIR "${CMAKE_BINARY_DIR}/boost" )
  file( MAKE_DIRECTORY "${BOOST_DIR}" )
  
  set( BOOST_BUILD_DIR "${BOOST_DIR}/build" )

  # copy the boost sources to the build directory
  # TODO: use out-of-source builds for boost as well
  file(
    COPY
      "${CMAKE_SOURCE_DIR}/deps/sources/boost"
    DESTINATION
      "${BOOST_BUILD_DIR}"
    FOLLOW_SYMLINK_CHAIN
  )

  set( BOOST_PREFIX_DIR "${BOOST_DIR}/prefix" )
  file( MAKE_DIRECTORY "${BOOST_PREFIX_DIR}" )

  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
    set( B2_TOOLSET "clang" )
  endif()

  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
    set( B2_TOOLSET "gcc" )
  endif()

  set( B2_USER_CONFIG "${BOOST_DIR}/user-config.jam" )

  # create the project configuration
  file( WRITE "${B2_USER_CONFIG}"
    "using ${B2_TOOLSET} : cmake : ${CMAKE_CXX_COMPILER} ;\n"
  )

  add_custom_command(
    OUTPUT
      "${BOOST_DIR}/stamp"
      "${BOOST_PREFIX_DIR}/lib/libboost_container.a"
    COMMAND
      ./bootstrap.sh
        "--prefix=${BOOST_PREFIX_DIR}"
        --without-icu --with-python=python3
        --with-libraries=container,headers
    COMMAND
      ./b2
        "--user-config=${B2_USER_CONFIG}"
        "toolset=${B2_TOOLSET}-cmake"
        install
    COMMAND
      ${CMAKE_COMMAND} -E touch "${BOOST_DIR}/stamp"
    WORKING_DIRECTORY "${BOOST_BUILD_DIR}/boost"
    COMMENT "Building Boost C++ Libraries"
    VERBATIM
  )
  add_custom_target(
    build-prtcl-dep-boost
    DEPENDS "${BOOST_DIR}/stamp"
    COMMENT "Target for building the Boost C++ Libraries"
  )

  add_library(
    prtcl-dep-boost-headers
    INTERFACE
  )
  add_dependencies(
    prtcl-dep-boost-headers
    build-prtcl-dep-boost
  )
  target_include_directories(
    prtcl-dep-boost-headers SYSTEM
    INTERFACE "${BOOST_PREFIX_DIR}/include"
  )

  add_library(
    prtcl-dep-boost-container STATIC
    IMPORTED
  )
  add_dependencies(
    prtcl-dep-boost-headers
    build-prtcl-dep-boost
  )
  set_property(
    TARGET prtcl-dep-boost-container
    PROPERTY IMPORTED_LOCATION "${BOOST_PREFIX_DIR}/lib/libboost_container.a"
  )
  target_link_libraries(
    prtcl-dep-boost-container
    INTERFACE prtcl-dep-boost-headers
  )
endfunction( prtcl_build_boost )

