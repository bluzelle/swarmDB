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

project(rocksdb CXX)

include(ExternalProject)
include(ProcessorCount)

# default to debug builds
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# rocksdb uses "levels"
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(DEBUG_LEVEL "2")
else()
  set(DEBUG_LEVEL "0")
endif()

set(PKG_VER "5.14.3")
string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
set(URL_HASH "c7019a645fc23df0adfe97ef08e793a36149bff2f57ef3b6174cbb0c8c9867b1")
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

ProcessorCount(NPROC)

ExternalProject_Add(rocksdb
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rocksdb"
    URL https://github.com/facebook/rocksdb/archive/v${PKG_VER}.tar.gz
    URL_HASH SHA256=${URL_HASH}
    TIMEOUT 30
    CONFIGURE_COMMAND ""
    BUILD_COMMAND PORTABLE=1 make -e DISABLE_JEMALLOC=1 DEBUG_LEVEL=${DEBUG_LEVEL} static_lib -j${NPROC}
    INSTALL_COMMAND make INSTALL_PATH=${PREFIX} install
    BUILD_IN_SOURCE true
    DOWNLOAD_NO_PROGRESS true
    )

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "14")
set(CPACK_PACKAGE_VERSION_PATCH "3")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}-${PKG_BUILD_TYPE})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE}/ DESTINATION ${PKG_BUILD_TYPE}/)

include(CPack)

message(STATUS "${PROJECT_NAME} -- ${CPACK_SYSTEM_NAME} ${PKG_BUILD_TYPE} ${PKG_VER}")
