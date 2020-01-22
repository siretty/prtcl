
function( prtcl_add_library NAME_ )
  # parse remaining function arguments
  set( l_options )
  set( l_ov_kwargs TYPE )
  set( l_mv_kwargs PRIVATE_SOURCES INTERFACE_SOURCES )
  cmake_parse_arguments(
    PARSE_ARGV 1 l_arg
    "${l_options}" "${l_ov_kwargs}" "${l_mv_kwargs}"
  )

  add_library(
    "${NAME_}" "${l_arg_TYPE}"
    ${l_arg_PRIVATE_SOURCES}
  )
  target_sources(
    "${NAME_}"
    INTERFACE ${l_arg_INTERFACE_SOURCES}
  )
  if( NOT "${l_arg_TYPE}" STREQUAL "INTERFACE" )
    DE_CXX17_Target( "${NAME_}" )                               
    DE_LibCXX_Target( "${NAME_}" )                              
    DE_Common_Diagnostics_CXX_Target( "${NAME_}" )              
    DE_Add_Our_Include_Directories( "${NAME_}" include sources )
  endif()
endfunction( prtcl_add_library )

