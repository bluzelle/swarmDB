add_library(crud STATIC
        crud.cpp
        crud.hpp
        crud_base.hpp
        )

target_link_libraries(crud)
target_include_directories(crud PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)
