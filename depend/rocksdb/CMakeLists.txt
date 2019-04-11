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

include(FindZLIB)
include(FindPackageHandleStandardArgs)

# find snappy...
find_path(SNAPPY_INCLUDE_DIR
    NAMES snappy.h
    HINTS ${SNAPPY_ROOT_DIR}/include)

find_library(SNAPPY_LIBRARIES
    NAMES snappy
    HINTS ${SNAPPY_ROOT_DIR}/lib)

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

find_package_handle_standard_args(Bzip2 DEFAULT_MSG
    BZIP2_LIBRARIES
    BZIP2_INCLUDE_DIR)

mark_as_advanced(
    BZIP2_ROOT_DIR
    BZIP2_LIBRARIES
    BZIP2_INCLUDE_DIR)

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/rocksdb/src/rocksdb)

set(ROCKSDB_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE INTERNAL "")
set(ROCKSDB_LIBRARIES ${SOURCE_DIR}/lib/librocksdb.a ${SNAPPY_LIBRARIES} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} CACHE INTERNAL "")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
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

    set(ROCKSDB_LIBRARIES ${ROCKSDB_LIBRARIES} CACHE INTERNAL "")
endif()
