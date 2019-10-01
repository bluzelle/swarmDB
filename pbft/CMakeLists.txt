add_library(pbft STATIC
    pbft_base.hpp
    pbft.hpp
    pbft.cpp
    pbft_configuration.hpp
    pbft_configuration.cpp
    dummy_pbft_service.cpp
    dummy_pbft_service.hpp
    pbft_service_base.hpp
    pbft_configuration.hpp
    pbft_configuration.cpp
    pbft_config_store.hpp
    pbft_config_store.cpp
    pbft_checkpoint_manager.cpp
    pbft_checkpoint_manager.hpp
    database_pbft_service.cpp
    database_pbft_service.hpp
    pbft_persistent_state.cpp)

target_link_libraries(pbft utils pbft_operations proto)
target_include_directories(pbft PRIVATE ${BLUZELLE_STD_INCLUDES})
add_dependencies(pbft boost openssl)

add_subdirectory(test)
add_subdirectory(operations)
