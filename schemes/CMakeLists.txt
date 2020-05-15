
project(
  prtcl-schemes
  VERSION 0.1
  LANGUAGES C CXX
)


include( PRTCL_Add_Target_CXX_FLAGS_FastMath )
include( PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative )


set( _PRJ ${CMAKE_CURRENT_SOURCE_DIR} )
set( _INC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set( _SRC ${CMAKE_CURRENT_SOURCE_DIR}/sources )

set( _BLD ${CMAKE_CURRENT_BINARY_DIR} )


find_package( Threads REQUIRED )
find_package( OpenMP REQUIRED )


find_program(
  PRTCL_CLANG_FORMAT
  NAMES clang-format
)

function( prtcl_generate_scheme_header _NAME )
  file( MAKE_DIRECTORY ${_BLD}/prtcl/schemes )

  add_custom_command(
    OUTPUT  ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
    COMMAND prtcl-gt-schemes ${_PRJ}/../share/schemes/${_NAME}.prtcl ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp ${_NAME} prtcl schemes
    DEPENDS prtcl-gt-schemes ${_PRJ}/../share/schemes/${_NAME}.prtcl
    COMMENT "Generating ${_NAME} scheme code."
  )

  if (PRTCL_CLANG_FORMAT)
    add_custom_command(
      OUTPUT  ${_INC}/prtcl/schemes/${_NAME}.hpp
      COMMAND ${PRTCL_CLANG_FORMAT} ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp > ${_INC}/prtcl/schemes/${_NAME}.hpp
      DEPENDS ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
      COMMENT "Formatting generated ${_NAME} scheme code."
    )
  else ()
    add_custom_command(
      OUTPUT  ${_INC}/prtcl/schemes/${_NAME}.hpp
      COMMAND ${CMAKE_COMMAND} -E copy ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp ${_INC}/prtcl/schemes/${_NAME}.hpp
      DEPENDS ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
      COMMENT "Not formatting generated ${_NAME} scheme code."
    )
  endif (PRTCL_CLANG_FORMAT)
endfunction( prtcl_generate_scheme_header )


prtcl_generate_scheme_header( density )
prtcl_generate_scheme_header( gravity )
prtcl_generate_scheme_header( viscosity )
prtcl_generate_scheme_header( correction )

prtcl_generate_scheme_header( symplectic_euler )

prtcl_generate_scheme_header( sesph )
prtcl_generate_scheme_header( iisph )

prtcl_generate_scheme_header( aiast12 )
prtcl_generate_scheme_header( aat13 )
prtcl_generate_scheme_header( he14 )
prtcl_generate_scheme_header( pt16 )
prtcl_generate_scheme_header( wkbb18 )

prtcl_add_library(
  prtcl-schemes
  TYPE INTERFACE
  INTERFACE_SOURCES
    ${_INC}/prtcl/schemes/density.hpp
    ${_INC}/prtcl/schemes/gravity.hpp
    ${_INC}/prtcl/schemes/viscosity.hpp
    ${_INC}/prtcl/schemes/correction.hpp

    ${_INC}/prtcl/schemes/symplectic_euler.hpp

    ${_INC}/prtcl/schemes/sesph.hpp
    ${_INC}/prtcl/schemes/iisph.hpp

    ${_INC}/prtcl/schemes/aiast12.hpp
    ${_INC}/prtcl/schemes/aat13.hpp
    ${_INC}/prtcl/schemes/he14.hpp
    ${_INC}/prtcl/schemes/pt16.hpp
    ${_INC}/prtcl/schemes/wkbb18.hpp
)
target_include_directories( prtcl-schemes INTERFACE ${_INC} )
target_link_libraries( prtcl-schemes
  INTERFACE prtcl-rt
)


#DE_Add_Catch2_Test(
#  prtcl-schemes-tests test_main
#
#  ${_TRACY_CPP} # tracing
#
#  ${_PRJ}/tests/prtcl/schemes/test_counting_particles.cpp
#)
#DE_CXX17_Target( prtcl-schemes-tests )
#DE_Common_Diagnostics_CXX_Target( prtcl-schemes-tests )
#DE_LibCXX_Target( prtcl-schemes-tests )
#DE_Add_Our_Include_Directories( prtcl-schemes-tests include sources )
#target_compile_definitions(
#  prtcl-schemes-tests
#  PUBLIC
#    #PRTCL_RT_LOG_TRACE_TRACY
#)
#target_include_directories(
#  prtcl-schemes-tests
#  PUBLIC
#    ${_TRACY_DIR}
#)
#target_link_libraries(
#  prtcl-schemes-tests
#  PUBLIC prtcl-schemes
#  PUBLIC prtcl-rt
#  PUBLIC Eigen3::Eigen
#  PUBLIC Threads::Threads
#  PUBLIC OpenMP::OpenMP_CXX
#  PUBLIC ${CMAKE_DL_LIBS}
#)


add_executable(
  prtcl-schemes-sesph

  ${_TRACY_CPP} # tracing

  ${_PRJ}/sources/prtcl-schemes-sesph.cpp
)
DE_CXX17_Target( prtcl-schemes-sesph )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-sesph )
DE_LibCXX_Target( prtcl-schemes-sesph )
DE_Add_Our_Include_Directories( prtcl-schemes-sesph include sources )
PRTCL_Add_Target_CXX_FLAGS_FastMath( prtcl-schemes-sesph )
PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative( prtcl-schemes-sesph )
target_compile_definitions(
  prtcl-schemes-sesph
  PUBLIC
    #PRTCL_RT_LOG_TRACE_TRACY
)
target_include_directories(
  prtcl-schemes-sesph
  PUBLIC
    ${_TRACY_DIR}
)
target_link_libraries(
  prtcl-schemes-sesph
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-schemes-iisph

  ${_TRACY_CPP} # tracing

  ${_PRJ}/sources/prtcl-schemes-iisph.cpp
)
DE_CXX17_Target( prtcl-schemes-iisph )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-iisph )
DE_LibCXX_Target( prtcl-schemes-iisph )
DE_Add_Our_Include_Directories( prtcl-schemes-iisph include sources )
PRTCL_Add_Target_CXX_FLAGS_FastMath( prtcl-schemes-iisph )
PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative( prtcl-schemes-iisph )
target_compile_definitions(
  prtcl-schemes-iisph
  PUBLIC
    #PRTCL_RT_LOG_TRACE_TRACY
)
target_include_directories(
  prtcl-schemes-iisph
  PUBLIC
    ${_TRACY_DIR}
)
target_link_libraries(
  prtcl-schemes-iisph
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-schemes-iisph-pt16

  ${_TRACY_CPP} # tracing

  ${_PRJ}/sources/prtcl-schemes-iisph-pt16.cpp
)
DE_CXX17_Target( prtcl-schemes-iisph-pt16 )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-iisph-pt16 )
DE_LibCXX_Target( prtcl-schemes-iisph-pt16 )
DE_Add_Our_Include_Directories( prtcl-schemes-iisph-pt16 include sources )
PRTCL_Add_Target_CXX_FLAGS_FastMath( prtcl-schemes-iisph-pt16 )
PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative( prtcl-schemes-iisph-pt16 )
target_compile_definitions(
  prtcl-schemes-iisph-pt16
  PUBLIC
    #PRTCL_RT_LOG_TRACE_TRACY
)
target_include_directories(
  prtcl-schemes-iisph-pt16
  PUBLIC
    ${_TRACY_DIR}
)
target_link_libraries(
  prtcl-schemes-iisph-pt16
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)


add_executable(
  prtcl-schemes-iisph-wkbb18

  ${_TRACY_CPP} # tracing

  ${_PRJ}/sources/prtcl-schemes-iisph-wkbb18.cpp
)
DE_CXX17_Target( prtcl-schemes-iisph-wkbb18 )
DE_Common_Diagnostics_CXX_Target( prtcl-schemes-iisph-wkbb18 )
DE_LibCXX_Target( prtcl-schemes-iisph-wkbb18 )
DE_Add_Our_Include_Directories( prtcl-schemes-iisph-wkbb18 include sources )
PRTCL_Add_Target_CXX_FLAGS_FastMath( prtcl-schemes-iisph-wkbb18 )
PRTCL_Add_Target_CXX_FLAGS_OptimizeForNative( prtcl-schemes-iisph-wkbb18 )
target_compile_definitions(
  prtcl-schemes-iisph-wkbb18
  PUBLIC
    #PRTCL_RT_LOG_TRACE_TRACY
)
target_include_directories(
  prtcl-schemes-iisph-wkbb18
  PUBLIC
    ${_TRACY_DIR}
)
target_link_libraries(
  prtcl-schemes-iisph-wkbb18
  PUBLIC prtcl-schemes
  PUBLIC prtcl-rt
  PUBLIC Eigen3::Eigen
  PUBLIC Threads::Threads
  PUBLIC OpenMP::OpenMP_CXX
  PUBLIC ${CMAKE_DL_LIBS}
)

