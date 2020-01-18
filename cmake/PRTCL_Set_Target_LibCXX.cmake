
# check if the selected C++ compiler is clang
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set( _is_clang TRUE )
else()
  set( _is_clang FALSE )
endif()

# create empty variables
set( PRTCL_LibCXX_CXX_FLAGS )
set( PRTCL_LibCXX_LINK_FLAGS )

# ------------------------------------------------------------
# Only Clang
# ------------------------------------------------------------
if( "${ENABLE_LIBCXX}" AND ${_is_clang} )
  list( APPEND PRTCL_LibCXX_CXX_FLAGS "-stdlib=libc++" )
  list( APPEND PRTCL_LibCXX_LINK_FLAGS "-stdlib=libc++" )
endif()

unset( _is_clang )

function( PRTCL_Set_Target_LibCXX _NAME )
  target_compile_options(
    ${_NAME}
    PUBLIC
    "${PRTCL_LibCXX_CXX_FLAGS}"
  )

  target_link_options(
    ${_NAME}
    PUBLIC
    "${PRTCL_LibCXX_LINK_FLAGS}"
  )
endfunction( PRTCL_Set_Target_LibCXX _NAME )

