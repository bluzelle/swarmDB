message( "Building version: ${PROJECT_VERSION}..." )

set(CPACK_PACKAGE_VENDOR              "Bluzelle")
set(CPACK_PACKAGE_CONTACT             "devops@bluzelle.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Bluzelle decentralized database node.")
set(CPACK_PACKAGE_NAME                "bluzelle-swarmdb")
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR       ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR       ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH       ${PROJECT_VERSION_PATCH})

set(CPACK_SYSTEM_NAME                 "amd64")

set(CPACK_GENERATOR                   "TGZ")
set(CPACK_PACKAGE_FILE_NAME            ${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_darwin_${CPACK_SYSTEM_NAME})

INCLUDE(CPack)

set(DESTINATION_DIR ./)

install(PROGRAMS  "${PROJECT_BINARY_DIR}/output/swarm" DESTINATION "./")