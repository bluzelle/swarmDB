add_library(pbft STATIC
        pbft_base.hpp
        pbft.hpp
        pbft.cpp

        pbft_operation.hpp
        pbft_operation.cpp

        pbft_configuration.hpp
        pbft_configuration.cpp

        dummy_pbft_service.cpp
        dummy_pbft_service.hpp
        pbft_service_base.hpp

        pbft_failure_detector.cpp
        pbft_failure_detector.hpp
        pbft_failure_detector_base.hpp pbft_configuration.hpp pbft_configuration.cpp test/pbft_configuration_test.cpp pbft_config_store.hpp pbft_config_store.cpp test/pbft_config_store_test.cpp)

target_link_libraries(pbft utils proto)
target_include_directories(pbft PRIVATE ${JSONCPP_INCLUDE_DIRS} ${PROTO_INCLUDE_DIR})
add_dependencies(pbft openssl)

add_subdirectory(test)
