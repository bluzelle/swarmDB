add_library(audit
        audit_base.hpp
        audit.hpp
        audit.cpp
    ../mocks/mock_pbft_base.hpp)

target_link_libraries(audit utils proto)
add_dependencies(audit boost jsoncpp)
target_include_directories(audit PRIVATE ${BLUZELLE_STD_INCLUDES})
add_subdirectory(test)
