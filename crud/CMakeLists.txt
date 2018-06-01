add_library(crud STATIC
        crud.cpp
        crud.hpp
        crud_base.hpp
        )

target_link_libraries(crud proto)
target_include_directories(crud PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})

add_subdirectory(test)
