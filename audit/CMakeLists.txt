add_library(audit
        audit_base.hpp
        audit.hpp
        audit.cpp
        )

target_link_libraries(audit proto)
target_include_directories(audit PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})
add_subdirectory(test)
