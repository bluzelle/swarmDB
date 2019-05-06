add_library(pbft_operations STATIC
    pbft_operation.hpp
    pbft_operation.cpp
    pbft_memory_operation.hpp
    pbft_memory_operation.cpp
    pbft_persistent_operation.cpp
    pbft_persistent_operation.cpp
    pbft_operation_manager.hpp
    pbft_operation_manager.cpp
    )

target_link_libraries(pbft_operations utils proto pbft)
target_include_directories(pbft_operations PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)
