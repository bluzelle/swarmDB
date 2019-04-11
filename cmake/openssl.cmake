# Copyright (C) 2018 Bluzelle
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

include(ExternalProject)
include(ProcessorCount)

# Prevent travis gcc crashes...
if (DEFINED ENV{TRAVIS})
    set(BUILD_FLAGS -j8)
else()
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(BUILD_FLAGS -j${N})
    endif()
endif()

# platform detection
if (APPLE)
    set(OPENSSL_BUILD_PLATFORM darwin64-x86_64-cc)
else()
    set(OPENSSL_BUILD_PLATFORM linux-generic64)
endif()

ExternalProject_Add(openssl
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/openssl"
    URL https://www.openssl.org/source/openssl-1.1.1.tar.gz
    URL_HASH SHA256=2836875a0f89c03d0fdf483941512613a50cfb421d6fd94b9f41d7279d586a3d
    TIMEOUT 30
    INSTALL_COMMAND ""
    DOWNLOAD_NO_PROGRESS true
    CONFIGURE_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/openssl/src/openssl/Configure" ${OPENSSL_BUILD_PLATFORM}
    BUILD_COMMAND make ${BUILD_FLAGS}
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/openssl")

ExternalProject_Get_Property(openssl source_dir)
ExternalProject_Get_Property(openssl binary_dir)

set(OPENSSL_INCLUDE_DIR ${source_dir}/include ${binary_dir}/include)
set(OPENSSL_LIBRARIES ${binary_dir}/libcrypto.a ${binary_dir}/libssl.a dl pthread)
