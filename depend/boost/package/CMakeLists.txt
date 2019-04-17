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

project(boost CXX)

include(ExternalProject)
include(ProcessorCount)

# default to debug builds
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(PKG_VER "1.68.0")
string(TOLOWER ${CMAKE_BUILD_TYPE} PKG_BUILD_TYPE)
set(URL_HASH "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf")
set(PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE})

set(BOOST_TARBALL "boost_${PKG_VER}")
string(REPLACE "." "_" BOOST_TARBALL ${BOOST_TARBALL})
string(APPEND BOOST_TARBALL ".tar.gz")
set(BOOST_LIBS "chrono,program_options,random,regex,system,thread,log,serialization")

ProcessorCount(NPROC)

ExternalProject_Add(boost
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/boost
    URL https://dl.bintray.com/boostorg/release/${PKG_VER}/source/${BOOST_TARBALL}
    URL_HASH SHA256=${URL_HASH}
    TIMEOUT 120
    CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/bootstrap.sh --prefix=${PREFIX} --with-libraries=${BOOST_LIBS}
    BUILD_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/b2 variant=${PKG_BUILD_TYPE} link=static -j${NPROC} install
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE true
    DOWNLOAD_NO_PROGRESS true
    )

set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "68")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}-${PKG_BUILD_TYPE})
string(TOLOWER ${CMAKE_SYSTEM_NAME} CPACK_SYSTEM_NAME)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_${PKG_BUILD_TYPE}/ DESTINATION ${PKG_BUILD_TYPE}/)

include(CPack)

message(STATUS "${PROJECT_NAME} -- ${CPACK_SYSTEM_NAME} ${PKG_BUILD_TYPE} ${PKG_VER} (${BOOST_LIBS})")
