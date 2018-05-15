add_library(ethereum STATIC
    ethereum.hpp
    ethereum.cpp
    ethereum_base.hpp
  )

target_link_libraries(ethereum)
target_include_directories(ethereum PRIVATE ${JSONCPP_INCLUDE_DIRS})

add_subdirectory(test)
