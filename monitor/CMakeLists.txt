add_library(monitor
        monitor_base.hpp
        monitor.hpp
        monitor.cpp
        )

add_dependencies(monitor boost jsoncpp)
target_include_directories(monitor PRIVATE ${BLUZELLE_STD_INCLUDES})
target_link_libraries(utils)
add_subdirectory(test)
