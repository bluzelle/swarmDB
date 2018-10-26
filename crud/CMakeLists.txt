add_library(crud STATIC
    raft_crud.cpp
    raft_crud.hpp
    crud_base.hpp
    subscription_manager.cpp
    subscription_manager.hpp
    subscription_manager_base.hpp
    crud.cpp
    crud.hpp
    )

target_link_libraries(crud proto)
add_dependencies(crud jsoncpp openssl)
target_include_directories(crud PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})

add_subdirectory(test)
