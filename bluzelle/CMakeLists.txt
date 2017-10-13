cmake_minimum_required(VERSION 3.7)
project(bluzelle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#set(wxWidgets_ROOT_DIR </usr/include/wx-3.0-unofficial>) # here I am  giving where to search for wxwidgets library. it can be different for you
set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})



find_package(Boost 1.62.0 COMPONENTS system filesystem locale REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(
        inc
        inc/gui_components
        inc/sim
        inc/containers
)

set(INCLUDE_FILES
        inc/sim/BZTask.h
        inc/bluzelleLogo.xpm.h
        inc/BZApplication.h
        inc/BZRootFrame.h
        inc/sim/ThreadData.h
        inc/gui_components/BZVisualizationPanel.h
        inc/sim/ThreadManager.h
        inc/gui_components/BZDataSource.h
        inc/gui_components/BZNodeListView.h
        inc/sim/BZTask.h
        inc/gui_components/BZStaticText.h
        inc/gui_components/BZApplicationLogListView.h
        inc/containers/BZSyncSetWrapper.hpp
        inc/containers/BZSyncMapWrapper.hpp
        inc/containers/CQueue.h
        )

set(SOURCE_FILES
        src/sim/BZTask.cpp
        src/main.cpp
        src/BZApplication.cpp
        src/BZRootFrame.cpp
        src/gui_components/BZVisualizationPanel.cpp
        src/sim/ThreadManager.cpp
        src/gui_components/BZDataSource.cpp
        src/gui_components/BZStaticText.cpp
        src/gui_components/BZNodeListView.cpp
        src/gui_components/BZApplicationLogListView.cpp
        src/gui_components/BZEthereumAddressDialog.cpp inc/gui_components/BZEthereumAddressDialog.h)

add_executable(bluzelle ${SOURCE_FILES} ${INCLUDE_FILES})
target_link_libraries(bluzelle ${wxWidgets_LIBRARIES})
target_link_libraries(bluzelle ${Boost_LIBRARIES})
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(bluzelle rt)
endif()


###############################################################################
## Unit Tests.
find_package(Boost 1.62 COMPONENTS unit_test_framework REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

enable_testing()

set(SOURCE_FILES
        inc/sim/ThreadData.h
        src/test.cpp
        src/sim/ThreadData.tests.cpp
        src/containers/BZSyncMapWrapper.tests.cpp
        src/containers/BZSyncSetWrapper.tests.cpp inc/containers/CQueue.h src/containers/BZSyncQueue.tests.cpp)
add_executable(bluzelle_test ${SOURCE_FILES})
target_link_libraries(bluzelle_test ${Boost_LIBRARIES})
