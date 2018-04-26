add_library(options STATIC
        ../include/bluzelle.hpp
        options.cpp
        options.hpp
        options_base.hpp
        )

target_link_libraries(options)
target_include_directories(options PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)
