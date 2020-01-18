
function( PRTCL_Set_Target_CXX14 _NAME )
  set_target_properties(
    ${_NAME}
    PROPERTIES
    CXX_STANDARD 14
    CXX_EXTENSIONS OFF
  )
endfunction( PRTCL_Set_Target_CXX14 )

function( PRTCL_Set_Target_CXX17 _NAME )
  set_target_properties(
    ${_NAME}
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
  )
endfunction( PRTCL_Set_Target_CXX17 )

