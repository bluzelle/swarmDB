# Copyright (C) 2019 Bluzelle
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License, version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)

include(ExternalProject)

macro(add_external_project NAME VERSION)
    message(STATUS "${NAME} -- ${CMAKE_SYSTEM_NAME} ${CMAKE_BUILD_TYPE} ${VERSION}")

    add_subdirectory(${NAME})

    string(TOLOWER ${CMAKE_SYSTEM_NAME} PKG_SYSTEM_NAME)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
        set(PKG_BUILD_TYPE "release")
    else()
        string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
    endif()

    ExternalProject_Add(${NAME}
        URL file://${CMAKE_SOURCE_DIR}/depend/${NAME}/package/${NAME}-${PKG_BUILD_TYPE}-${VERSION}-${PKG_SYSTEM_NAME}.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        PREFIX ${CMAKE_BINARY_DIR}/${NAME}
        INSTALL_COMMAND ""
        )
endmacro()

add_external_project(boost 1.70.0)
add_external_project(googletest 1.8.0)
add_external_project(jsoncpp 1.8.4)
add_external_project(openssl 1.1.1)
add_external_project(rocksdb 5.14.3)
