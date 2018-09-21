add_library(storage STATIC
    mem_storage.cpp
    mem_storage.hpp
    storage_base.hpp
    rocksdb_storage.hpp
    rocksdb_storage.cpp)

target_link_libraries(storage)
add_dependencies(storage jsoncpp)
target_include_directories(storage PRIVATE ${JSONCPP_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIRS})

add_subdirectory(test)
