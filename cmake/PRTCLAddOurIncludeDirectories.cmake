
function( PRTCL_Add_Our_Include_Directories _NAME _INCLUDE_DIR _SOURCES_DIR )
  target_include_directories(
    ${_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/${_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${_SOURCES_DIR}
  )
endfunction( PRTCL_Add_Our_Include_Directories _NAME )

