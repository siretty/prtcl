
function( DE_Add_Catch2_Main _NAME )
  file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/DE_Add_Catch2_Main.cpp
    "#define CATCH_CONFIG_MAIN\n"
    "#include <catch2/catch.hpp>\n"
  )

  add_library(
    ${_NAME} STATIC

    # sources
    ${CMAKE_CURRENT_BINARY_DIR}/DE_Add_Catch2_Main.cpp
  )

  target_link_libraries(
    ${_NAME}
    PUBLIC Catch2::Catch2
  )
endfunction( DE_Add_Catch2_Main )

function( DE_Add_Catch2_Test _NAME _MAIN )
  add_executable(
    ${_NAME}

    # sources
    ${ARGN}
  )

  target_link_libraries(
    ${_NAME}

    # link to the Catch2 main library
    PUBLIC ${_MAIN}
  )
endfunction( DE_Add_Catch2_Test )

