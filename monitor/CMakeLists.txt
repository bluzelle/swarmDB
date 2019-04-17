add_library(monitor
        monitor_base.hpp
        monitor.hpp
        monitor.cpp
        )

add_dependencies(monitor boost)
target_include_directories(monitor PRIVATE ${JSONCPP_INCLUDE_DIRS} ${GTEST_INCLUDE_DIR})
target_link_libraries(utils)
add_subdirectory(test)
