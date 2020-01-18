
function( PRTCL_Add_Catch2_Main _NAME )
  file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/DE_Add_Catch2_Main.cpp
    "#define CATCH_CONFIG_MAIN\n"
    "#include <catch.hpp>\n"
  )

  add_library(
    ${_NAME} STATIC

    # sources
    ${CMAKE_CURRENT_BINARY_DIR}/PRTCL_Add_Catch2_Main.cpp
  )

  # add catch2 includes
  target_include_directories(
    ${_NAME} 
    PRIVATE ${Catch2_SOURCE_DIR}/single_include/catch2
  )
endfunction( PRTCL_Add_Catch2_Main )

function( PRTCL_Add_Catch2_Test _NAME _MAIN )
  add_executable(
    ${_NAME}

    # sources
    ${ARGN}
  )

  # add catch2 includes
  target_include_directories(
    ${_NAME} 
    PRIVATE ${Catch2_SOURCE_DIR}/single_include/catch2
  )

  target_link_libraries(
    ${_NAME}

    # link to the Catch2 main library
    PUBLIC ${_MAIN}
  )
endfunction( PRTCL_Add_Catch2_Test )

