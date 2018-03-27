include(ExternalProject)

ExternalProject_Add(googletest
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest"
        GIT_REPOSITORY https://github.com/google/googletest.git
        INSTALL_DIR "${CMAKE_BINARY_DIR}"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}"
        GIT_TAG release-1.8.0
        GIT_SHALLOW true)

include_directories(${CMAKE_BINARY_DIR}/include)
link_directories(${CMAKE_BINARY_DIR}/lib)
set(GMOCK_BOTH_LIBRARIES gmock_main gmock)
