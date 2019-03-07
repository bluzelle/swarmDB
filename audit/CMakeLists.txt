add_library(audit
        audit_base.hpp
        audit.hpp
        audit.cpp
    ../mocks/mock_pbft_base.hpp)

target_link_libraries(audit utils proto)
add_dependencies(audit jsoncpp)
target_include_directories(audit PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})
add_subdirectory(test)
