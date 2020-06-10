
function(prtcl_add_library NAME_)
  # parse remaining function arguments
  set(l_options)
  set(l_ov_kwargs TYPE)
  set(l_mv_kwargs PRIVATE_SOURCES INTERFACE_SOURCES PUBLIC_HEADERS)
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

  if (l_arg_PUBLIC_HEADERS)
    set_property(
        TARGET "${NAME_}"
        PROPERTY PUBLIC_HEADER
        ${l_arg_PUBLIC_HEADERS}
    )
  endif ()

  if (NOT "${l_arg_TYPE}" STREQUAL "INTERFACE")
    DE_CXX17_Target("${NAME_}")
    DE_LibCXX_Target("${NAME_}")
    DE_Common_Diagnostics_CXX_Target("${NAME_}")
    DE_Add_Our_Include_Directories("${NAME_}" include sources)
  endif ()
endfunction(prtcl_add_library)


function(prtcl_add_library_simple NAME_)
  # parse remaining function arguments
  set(l_options)
  set(l_ov_kwargs TYPE)
  set(l_mv_kwargs
      SOURCES_DIR HEADERS_DIR
      PHYSICAL_COMPONENTS
      EXTRA_HEADERS EXTRA_SOURCES
      LINK_LIBRARIES)

  cmake_parse_arguments(
      PARSE_ARGV 1 l_arg
      "${l_options}" "${l_ov_kwargs}" "${l_mv_kwargs}"
  )

  set(include_directories)

  if (l_arg_SOURCES_DIR)
    set(S "${l_arg_SOURCES_DIR}")
  else ()
    set(S "${CMAKE_CURRENT_SOURCE_DIR}")
  endif ()
  list(APPEND include_directories "${S}")

  if (l_arg_HEADERS_DIR)
    set(I "${l_arg_HEADERS_DIR}")
    list(APPEND include_directories "${I}")
  else ()
    set(I "${l_arg_SOURCES_DIR}")
  endif ()

  set(sources "${l_arg_PHYSICAL_COMPONENTS}")
  list(TRANSFORM sources PREPEND "${S}/")
  list(TRANSFORM sources APPEND ".cpp")
  list(APPEND sources ${EXTRA_SOURCES})

  set(headers "${libprtcl_components}")
  list(TRANSFORM headers PREPEND "${I}/")
  list(TRANSFORM headers APPEND ".hpp")
  list(APPEND headers ${EXTRA_HEADERS})

  add_library(
      "${NAME_}" "${l_arg_TYPE}"
  )

  target_include_directories(
      ${NAME_} PUBLIC
      $<BUILD_INTERFACE:${include_directories}>
      $<INSTALL_INTERFACE:include/prtcl>
  )

  target_sources(
      "${NAME_}"
      PRIVATE ${sources}
  )

  if (headers)
    target_sources(
        "${NAME_}"
        PRIVATE ${headers}
    )

    set_property(
        TARGET "${NAME_}"
        PROPERTY PUBLIC_HEADER
        ${headers} ${l_arg_EXTRA_HEADERS}
    )
  endif ()

  if (l_arg_LINK_LIBRARIES)
    target_link_libraries(
        "${NAME_}"
        ${l_arg_LINK_LIBRARIES}
    )
  endif ()

  set_target_properties(
      "${NAME_}" PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )

  if (NOT "${l_arg_TYPE}" STREQUAL "INTERFACE")
    DE_Common_Diagnostics_CXX_Target("${NAME_}")
  endif ()
endfunction(prtcl_add_library_simple)

