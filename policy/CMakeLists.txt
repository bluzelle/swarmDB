add_library(policy STATIC
        eviction_base.hpp
        random.cpp
        volatile_ttl.cpp
        )

add_dependencies(policy proto)

target_include_directories(policy PRIVATE ${BLUZELLE_STD_INCLUDES})

add_subdirectory(test)
