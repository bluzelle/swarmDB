add_library(crud STATIC
    crud_base.hpp
    subscription_manager.cpp
    subscription_manager.hpp
    subscription_manager_base.hpp
    crud.cpp
    crud.hpp
    )

target_link_libraries(crud proto policy)
add_dependencies(crud boost jsoncpp openssl)
target_include_directories(crud PRIVATE ${BLUZELLE_STD_INCLUDES})
add_subdirectory(test)
