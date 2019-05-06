add_library(storage STATIC
    mem_storage.cpp
    mem_storage.hpp
    storage_base.hpp
    rocksdb_storage.hpp
    rocksdb_storage.cpp)

target_link_libraries(storage)
add_dependencies(storage boost jsoncpp rocksdb)
target_include_directories(storage PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)
