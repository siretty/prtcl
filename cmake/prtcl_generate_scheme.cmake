
function(prtcl_generate_scheme _NAME)
  set(_PRJ ${CMAKE_CURRENT_SOURCE_DIR})
  set(_BLD ${CMAKE_CURRENT_BINARY_DIR})

  set(_GEN ${CMAKE_SOURCE_DIR}/lua/prtcl-generate)

  file(MAKE_DIRECTORY ${_BLD}/prtcl/schemes)

  set(_PRTCL_FILE ${_PRJ}/../share/schemes/${_NAME}.prtcl)
  set(_HPP_FILE ${_PRJ}/prtcl/schemes/${_NAME}.hpp)
  set(_CPP_FILE ${_PRJ}/prtcl/schemes/${_NAME}.cpp)

  add_custom_command(
      OUTPUT ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
      COMMAND ${_GEN} cxx_openmp_impl_hpp ${_PRTCL_FILE} ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp ${_NAME} prtcl schemes
      DEPENDS ${_GEN} ${_PRTCL_FILE}
      COMMENT "Generating ${_NAME} scheme header (.hpp)."
  )

  if (PRTCL_CLANG_FORMAT)
    add_custom_command(
        OUTPUT ${_HPP_FILE}
        COMMAND ${PRTCL_CLANG_FORMAT} ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp > ${_HPP_FILE}
        DEPENDS ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
        COMMENT "Formatting generated ${_NAME} scheme code."
    )
  else ()
    add_custom_command(
        OUTPUT ${_HPP_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp ${_HPP_FILE}
        DEPENDS ${_BLD}/prtcl/schemes/${_NAME}.raw.hpp
        COMMENT "Not formatting generated ${_NAME} scheme code."
    )
  endif (PRTCL_CLANG_FORMAT)

  add_custom_command(
      OUTPUT ${_CPP_FILE}
      COMMAND ${_GEN} cxx_openmp_impl_cpp ${_PRTCL_FILE} ${_CPP_FILE} ${_NAME} prtcl schemes
      DEPENDS ${_GEN} ${_HPP_FILE}
      COMMENT "Generating ${_NAME} scheme source (.cpp)."
  )

  message(NOTICE "Header: ${_HPP_FILE}")
  message(NOTICE "Source: ${_CPP_FILE}")
endfunction(prtcl_generate_scheme)

