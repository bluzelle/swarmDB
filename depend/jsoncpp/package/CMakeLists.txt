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

project(jsoncpp CXX)

include(ExternalProject)

# default to debug builds
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(PKG_VER "1.8.4")
string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
set(URL_HASH "c49deac9e0933bcb7044f08516861a2d560988540b23de2ac1ad443b219afdb6")
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

message(STATUS "${PROJECT_NAME} -- ${PKG_BUILD_TYPE} ${PKG_VER}")

ExternalProject_Add(jsoncpp
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp"
    URL https://github.com/open-source-parsers/jsoncpp/archive/${PKG_VER}.tar.gz
    URL_HASH SHA256=${URL_HASH}
    TIMEOUT 30
    CMAKE_ARGS
	-DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF
	-DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
	-DJSONCPP_WITH_TESTS=OFF
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_INSTALL_PREFIX=${PREFIX}
    DOWNLOAD_NO_PROGRESS true
    )

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "8")
set(CPACK_PACKAGE_VERSION_PATCH "4")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}-${PKG_BUILD_TYPE})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE}/ DESTINATION ${PKG_BUILD_TYPE}/)

include(CPack)
