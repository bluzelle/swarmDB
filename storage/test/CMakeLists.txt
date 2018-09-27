set(test_srcs storage_test.cpp)
set(test_libs storage node)
set(test_deps rocksdb)
set(test_link ${ROCKSDB_LIBRARIES})

add_gmock_test(storage)
