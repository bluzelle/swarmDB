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

set(REQUIRED_BOOST "1.70.0")
set(BOOST_URL_HASH "882b48708d211a5f48e60b0124cf5863c1534cd544ecd0664bb534a4b5d506e9")

message(STATUS "Boost: ${REQUIRED_BOOST}")

set(BOOST_TARBALL "boost_${REQUIRED_BOOST}")
string(REPLACE "." "_" BOOST_TARBALL ${BOOST_TARBALL})
string(APPEND BOOST_TARBALL ".tar.gz")

set(BOOST_LIBS "chrono,program_options,random,regex,system,thread,log,serialization")

# Prevent travis gcc crashes...
if (DEFINED ENV{TRAVIS})
    set(BUILD_FLAGS -j8)
else()
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(BUILD_FLAGS -j${N})
    endif()
endif()

ExternalProject_Add(boost
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/boost"
    URL "https://dl.bintray.com/boostorg/release/${REQUIRED_BOOST}/source/${BOOST_TARBALL}"
    URL_HASH SHA256=${BOOST_URL_HASH}
    TIMEOUT 120
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/bootstrap.sh" "--with-libraries=${BOOST_LIBS}"
    BUILD_COMMAND "${CMAKE_CURRENT_BINARY_DIR}/boost/src/boost/b2" link=static "${BUILD_FLAGS} "
    BUILD_IN_SOURCE true
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM ${CMAKE_CURRENT_BINARY_DIR}/boost)

ExternalProject_Get_Property(boost source_dir)
set(Boost_INCLUDE_DIRS ${source_dir})

set(Boost_LIBRARIES
    ${source_dir}/stage/lib/libboost_log.a
    ${source_dir}/stage/lib/libboost_program_options.a
    ${source_dir}/stage/lib/libboost_system.a
    ${source_dir}/stage/lib/libboost_thread.a
    pthread
    ${source_dir}/stage/lib/libboost_serialization.a
    ${source_dir}/stage/lib/libboost_date_time.a
    ${source_dir}/stage/lib/libboost_log_setup.a
    ${source_dir}/stage/lib/libboost_filesystem.a
    ${source_dir}/stage/lib/libboost_regex.a
    ${source_dir}/stage/lib/libboost_chrono.a
    ${source_dir}/stage/lib/libboost_atomic.a
)
