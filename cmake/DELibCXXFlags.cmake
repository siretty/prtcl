
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set( _is_clang TRUE )
else()
  set( _is_clang FALSE )
endif()

set( DE_LibCXX_CXX_FLAGS )
set( DE_LibCXX_LINK_FLAGS )

# ------------------------------------------------------------
# Only Clang
# ------------------------------------------------------------
if( "${ENABLE_LIBCXX}" AND ${_is_clang} )
  list( APPEND DE_LibCXX_CXX_FLAGS "-stdlib=libc++" )
  list( APPEND DE_LibCXX_LINK_FLAGS "-stdlib=libc++" )
endif()

unset( _is_clang )
unset( _is_gcc )

function( DE_LibCXX_Target _NAME )
  target_compile_options(
    ${_NAME}
    PUBLIC
    "${DE_LibCXX_CXX_FLAGS}"
  )

  target_link_options(
    ${_NAME}
    PUBLIC
    "${DE_LibCXX_LINK_FLAGS}"
  )
endfunction( DE_LibCXX_Target _NAME )

