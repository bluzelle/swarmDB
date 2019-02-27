add_library(monitor
        monitor_base.hpp
        monitor.hpp
        monitor.cpp
        )

target_link_libraries(utils)
add_subdirectory(test)
