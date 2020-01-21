
option(
  OPTIMIZE_FOR_NATIVE
  "Build with flags optimizing for the native CPU (clang/gcc)."
  OFF
)

if( ${OPTIMIZE_FOR_NATIVE} )
  message("cmake/PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative: Compiling for the native CPU.")
endif()

# check if the C++ compiler is clang
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set( _is_clang TRUE )
else()
  set( _is_clang FALSE )
endif()

# check if the C++ compiler is gcc
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
  set( _is_gcc TRUE )
else()
  set( _is_gcc FALSE )
endif()

# ------------------------------------------------------------
# Both Clang and GCC
# ------------------------------------------------------------
if( ${_is_clang} OR ${_is_gcc} )
  set( _FLAGS )
  list( APPEND _FLAGS
    "-march=native"
  )
endif()

set( PRTCL_CXX_FLAGS_OptimizeForNative ${_FLAGS} )

# unset local variables
unset( _is_clang )
unset( _is_gcc )
unset( _FLAGS )

function( PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative _NAME )
  target_compile_options(
    ${_NAME}
    PUBLIC
    "${PRTCL_CXX_FLAGS_OptimizeForNative}"
  )
endfunction( PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative _NAME )


