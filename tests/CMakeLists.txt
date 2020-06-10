
prtcl_add_executable(
    testme

    SOURCES
    testme.cpp
)
target_link_libraries(testme PUBLIC libprtcl)
