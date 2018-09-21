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
include(FindZLIB)

# find snappy...
find_path(SNAPPY_INCLUDE_DIR
    NAMES snappy.h
    HINTS ${SNAPPY_ROOT_DIR}/include)

find_library(SNAPPY_LIBRARIES
    NAMES snappy
    HINTS ${SNAPPY_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Snappy DEFAULT_MSG
    SNAPPY_LIBRARIES
    SNAPPY_INCLUDE_DIR)

mark_as_advanced(
    SNAPPY_ROOT_DIR
    SNAPPY_LIBRARIES
    SNAPPY_INCLUDE_DIR)

# find bzip2...
find_path(BZIP2_INCLUDE_DIR
    NAMES bzlib.h
    HINTS ${BZIP2_ROOT_DIR}/include)

find_library(BZIP2_LIBRARIES
    NAMES bz2
    HINTS ${BZIP2_ROOT_DIR}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bzip2 DEFAULT_MSG
    BZIP2_LIBRARIES
    BZIP2_INCLUDE_DIR)

mark_as_advanced(
    BZIP2_ROOT_DIR
    BZIP2_LIBRARIES
    BZIP2_INCLUDE_DIR)

# Prevent travis gcc crashes...
if (DEFINED ENV{TRAVIS})
    set(BUILD_FLAGS -j8)
else()
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(BUILD_FLAGS -j${N})
    endif()
endif()

ExternalProject_Add(rocksdb
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rocksdb"
    URL https://github.com/facebook/rocksdb/archive/v5.14.3.tar.gz
    URL_HASH SHA256=c7019a645fc23df0adfe97ef08e793a36149bff2f57ef3b6174cbb0c8c9867b1
    TIMEOUT 30
    INSTALL_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make -e DISABLE_JEMALLOC=1 static_lib ${BUILD_FLAGS}
    BUILD_IN_SOURCE true
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/rocksdb")

ExternalProject_Get_Property(rocksdb source_dir)
set(ROCKSDB_INCLUDE_DIRS ${source_dir}/include)

ExternalProject_Get_Property(rocksdb binary_dir)
link_directories(${binary_dir}/)

set(ROCKSDB_LIBRARIES ${binary_dir}/librocksdb.a ${SNAPPY_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES})

if (APPLE)
    find_library(LZ4_LIBRARY NAMES liblz4.a)
    message(STATUS ${LZ4_LIBRARY})
    list(APPEND ROCKSDB_LIBRARIES ${LZ4_LIBRARY})

    # rocksdb may of found these libraries...
    find_library(ZSTD_LIBRARY NAMES libzstd.a)
    if (ZSTD_LIBRARY)
        message(STATUS ${ZSTD_LIBRARY})
       list(APPEND ROCKSDB_LIBRARIES ${ZSTD_LIBRARY})
    endif()

    find_library(TBB_LIBRARY NAMES libtbb.a)
    if (TBB_LIBRARY)
        message(STATUS ${TBB_LIBRARY})
        list(APPEND ROCKSDB_LIBRARIES ${TBB_LIBRARY})
    endif()
endif()
