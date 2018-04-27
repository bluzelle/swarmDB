if (UNIX AND NOT APPLE)
    find_program(LSB_RELEASE lsb_release)
    execute_process(COMMAND ${LSB_RELEASE} -is
        OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu")
        add_subdirectory(debian)
    endif()
endif()

add_subdirectory(docker)
