add_library(monitor
        monitor_base.hpp
        monitor.hpp
        monitor.cpp
        )

add_dependencies(monitor boost)
target_link_libraries(utils)
add_subdirectory(test)
