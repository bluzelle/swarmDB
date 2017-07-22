cmake_minimum_required(VERSION 3.7)
project(bluzelle_poc)

message(STATUS "*** CMakeLists for BluzellePoc")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(wxWidgets_ROOT_DIR </usr/include/wx-3.0-unofficial>) # here I am  giving where to search for wxwidgets library. it can be different for you
set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

find_package(Boost 1.62.0 COMPONENTS system filesystem locale REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(inc)

set(SOURCE_FILES
        inc/KeplerApplication.hpp
        inc/KeplerFrame.hpp
        inc/KeplerSynchronizedMapWrapper.hpp
        inc/KeplerSynchronizedSetWrapper.hpp
        inc/ThreadData.hpp
        src/KeplerApplication.cpp
        src/main.cpp
        src/KeplerFrame.cpp
        src/Utility.cpp
        src/ThreadUtilities.cpp
        inc/ThreadUtilities.h
        inc/ThreadManager.h
        src/ThreadManager.cpp
        inc/BZVisualizationPanel.h
        src/BZVisualizationPanel.cpp
        )

add_executable(bluzelle_poc ${SOURCE_FILES})
target_link_libraries(bluzelle_poc ${Boost_LIBRARIES})
target_link_libraries(bluzelle_poc ${wxWidgets_LIBRARIES})