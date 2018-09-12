add_library(utils STATIC
        http_get.cpp
        http_get.hpp
        blacklist.cpp
        blacklist.hpp
        make_endpoint.hpp
        make_endpoint.cpp
        crypto.cpp
        crypto.hpp
        container.hpp)

target_link_libraries(utils ${CURL_LIBRARIES} ${JSONCPP_LIBRARIES} OpenSSL::SSL)

add_dependencies(utils jsoncpp)
target_include_directories(utils PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)
