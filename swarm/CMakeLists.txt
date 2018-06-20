add_executable(swarm main.cpp)
target_include_directories(swarm PRIVATE ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(swarm node http raft crud options ethereum bootstrap storage proto protobuf ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} pthread)
