include(ExternalProject)

ExternalProject_Add(googletest
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest"
        GIT_REPOSITORY https://github.com/google/googletest.git
        INSTALL_COMMAND ""
        GIT_TAG release-1.8.0
        GIT_SHALLOW true)

ExternalProject_Get_Property(googletest source_dir)
include_directories(${source_dir}/googlemock/include ${source_dir}/googletest/include)

ExternalProject_Get_Property(googletest binary_dir)
link_directories(${binary_dir}/googlemock)

set(GMOCK_BOTH_LIBRARIES gmock_main gmock)
