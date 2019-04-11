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

project(openssl CXX)

include(ExternalProject)
include(ProcessorCount)

# default to debug builds
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(PKG_VER "1.1.1")
string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
set(URL_HASH "2836875a0f89c03d0fdf483941512613a50cfb421d6fd94b9f41d7279d586a3d")
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

# platform detection
if (APPLE)
    set(OPENSSL_BUILD_PLATFORM darwin64-x86_64-cc)
else()
    set(OPENSSL_BUILD_PLATFORM linux-generic64)
endif()

ProcessorCount(NPROC)

ExternalProject_Add(openssl
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/openssl"
    URL https://www.openssl.org/source/openssl-${PKG_VER}.tar.gz
    URL_HASH SHA256=${URL_HASH}
    TIMEOUT 30
    DOWNLOAD_NO_PROGRESS true
	CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl/Configure ${OPENSSL_BUILD_PLATFORM} --prefix=${PREFIX} --${PKG_BUILD_TYPE}
	INSTALL_COMMAND make install_sw
    BUILD_COMMAND make -j${NPROC}
    )

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}-${PKG_BUILD_TYPE})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE}/ DESTINATION ${PKG_BUILD_TYPE}/)

include(CPack)

message(STATUS "${PROJECT_NAME} -- ${CPACK_SYSTEM_NAME} ${PKG_BUILD_TYPE} ${PKG_VER}")
