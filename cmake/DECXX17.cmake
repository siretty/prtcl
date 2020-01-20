
function( DE_CXX17_Target _NAME )
  set_target_properties(
    ${_NAME}
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
  )
endfunction( DE_CXX17_Target )

