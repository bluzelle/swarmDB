message( "Building version: ${PROJECT_VERSION}..." )

set(BOOST_DEB_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

set(CPACK_PACKAGE_VENDOR              "Bluzelle")
set(CPACK_PACKAGE_CONTACT             "devops@bluzelle.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Bluzelle decentralized database node.")
set(CPACK_PACKAGE_NAME                "swarmdb")
set(CPACK_PACKAGE_VERSION             ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR       ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR       ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH       ${PROJECT_VERSION_PATCH})
set(CPACK_DEBIAN_PACKAGE_SECTION      "databases")

set(CPACK_SYSTEM_NAME                 "amd64")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

set(CPACK_DEBIAN_PACKAGE_PRIORITY     "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER   "Bluzelle <devops@bluzelle.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS      "libjsoncpp-dev")
set(CPACK_GENERATOR                   "DEB")
set(CPACK_PACKAGE_FILE_NAME            ${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE})
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${PROJECT_SOURCE_DIR}/pkg/debian/postinst;${PROJECT_SOURCE_DIR}/pkg/debian/postrm)

INCLUDE(CPack)

set(DESTINATION_DIR /usr/bin/swarmdb)

# for 16.04
execute_process(COMMAND chmod 755 ${PROJECT_SOURCE_DIR}/pkg/debian/postinst)
execute_process(COMMAND chmod 755 ${PROJECT_SOURCE_DIR}/pkg/debian/postrm)

install(PROGRAMS  "${PROJECT_BINARY_DIR}/output/swarm" DESTINATION "/usr/bin/")
