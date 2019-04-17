add_executable(swarm main.cpp)
add_dependencies(swarm boost jsoncpp rocksdb)
target_include_directories(swarm PRIVATE ${JSONCPP_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIRS} ${GTEST_INCLUDE_DIR})
target_link_libraries(swarm node pbft audit crud chaos options ethereum bootstrap storage crypto monitor proto ${Protobuf_LIBRARIES} status ${ROCKSDB_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} pthread)
