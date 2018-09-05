add_executable(swarm main.cpp)
add_dependencies(swarm jsoncpp) # rocksdb
target_include_directories(swarm PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(swarm node http raft pbft audit crud options ethereum bootstrap storage proto protobuf.a status ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} pthread)
