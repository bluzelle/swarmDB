add_executable(swarm main.cpp)
add_dependencies(swarm boost jsoncpp rocksdb)
target_include_directories(swarm PRIVATE ${JSONCPP_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIRS})
target_link_libraries(swarm node http raft pbft audit crud chaos options ethereum bootstrap storage crypto proto ${Protobuf_LIBRARIES} status ${ROCKSDB_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} pthread)
