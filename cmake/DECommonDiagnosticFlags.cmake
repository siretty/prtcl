
option(
  FORCE_COLORED_DIAGNOSTICS
  "Always produce colored diagnostic output when compiling (clang/gcc)."
  OFF
)

if( ${FORCE_COLORED_DIAGNOSTICS} )
  message("DECommonDiagnosticFlags: Enforcing colored diagnostics.")
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  set( _is_clang TRUE )
else()
  set( _is_clang FALSE )
endif()

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
  set( _is_gcc TRUE )
else()
  set( _is_gcc FALSE )
endif()

# ------------------------------------------------------------
# Both Clang and GCC
# ------------------------------------------------------------
if( ${_is_clang} OR ${_is_gcc} )
  set( _FLAGS "-Wall" "-Wextra" )
  list( APPEND _FLAGS
    "-Wnon-virtual-dtor" "-Wold-style-cast" "-Wcast-align"
    "-Wunused" "-Woverloaded-virtual" "-Wpedantic" "-Wconversion"
    "-Wnull-dereference" "-Wdouble-promotion" "-Wformat=2"
    "-Wsign-conversion"
  )
endif()

# ------------------------------------------------------------
# Only Clang
# ------------------------------------------------------------
if( ${_is_clang} )
  if( FORCE_COLORED_DIAGNOSTICS )
    list( APPEND _FLAGS
      "-fcolor-diagnostics"
      "-fno-builtin"
    )
  endif()

  list( APPEND _FLAGS
    "-Wshadow"
  )
endif()

# ------------------------------------------------------------
# Only GCC
# ------------------------------------------------------------
if( ${_is_gcc} )
  if( FORCE_COLORED_DIAGNOSTICS )
    list( APPEND _FLAGS
      "-fdiagnostics-color=always"
    )
  endif()

  list( APPEND _FLAGS
    "-Wduplicated-cond" "-Wduplicated-branches" "-Wlogical-op"
    "-Wshadow=compatible-local"
    #"-Wuseless-cast"  # super-annoying for templated code
  )
endif()

set( DE_COMMON_DIAGNOSTICS_FLAGS_CXX ${_FLAGS} )

# unset local variables
unset( _is_clang )
unset( _is_gcc )
unset( _FLAGS )

function( DE_Common_Diagnostics_CXX_Target _NAME )
  target_compile_options(
    ${_NAME}
    PUBLIC
    "${DE_COMMON_DIAGNOSTICS_FLAGS_CXX}"
  )
endfunction( DE_Common_Diagnostics_CXX_Target )

