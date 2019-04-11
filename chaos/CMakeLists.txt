add_library(chaos
        chaos.hpp
        chaos.cpp
        chaos_base.hpp
        )

target_link_libraries(chaos options)
target_include_directories(chaos PRIVATE ${JSONCPP_INCLUDE_DIRS})
add_dependencies(chaos boost openssl)
add_subdirectory(test)
