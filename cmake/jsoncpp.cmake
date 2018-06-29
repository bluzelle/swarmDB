include(ExternalProject)

ExternalProject_Add(jsoncpp
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp"
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp
        INSTALL_COMMAND ""
        GIT_TAG 1.8.4
        GIT_SHALLOW true)

ExternalProject_Get_Property(jsoncpp source_dir)
set(JSONCPP_INCLUDE_DIRS ${source_dir}/include)
include_directories(${JSONCPP_INCLUDE_DIRS})

ExternalProject_Get_Property(jsoncpp binary_dir)
link_directories(${binary_dir}/src/lib_json/)

set(JSONCPP_LIBRARIES ${binary_dir}/src/lib_json/libjsoncpp.a)
