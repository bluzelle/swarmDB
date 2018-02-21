cmake_minimum_required(VERSION 3.10)
project(bluzelle)
message(STATUS "*** adding daemon subdirectory")

# OMG this is AWESOME! Seriously, install ccache.
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

add_subdirectory(daemon)
