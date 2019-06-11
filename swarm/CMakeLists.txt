add_executable(swarm main.cpp)
add_dependencies(swarm boost jsoncpp rocksdb)
target_include_directories(swarm PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(swarm node pbft audit crud chaos options bootstrap storage crypto monitor proto ${Protobuf_LIBRARIES} status ${ROCKSDB_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} pthread)
