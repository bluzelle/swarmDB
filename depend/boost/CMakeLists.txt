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

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/boost/src/boost)

set(Boost_INCLUDE_DIRS ${SOURCE_DIR}/include CACHE INTERNAL "")

set(Boost_LIBRARIES
    ${SOURCE_DIR}/lib/libboost_log.a
    ${SOURCE_DIR}/lib/libboost_program_options.a
    ${SOURCE_DIR}/lib/libboost_system.a
    ${SOURCE_DIR}/lib/libboost_thread.a
    pthread
    ${SOURCE_DIR}/lib/libboost_serialization.a
    ${SOURCE_DIR}/lib/libboost_date_time.a
    ${SOURCE_DIR}/lib/libboost_log_setup.a
    ${SOURCE_DIR}/lib/libboost_filesystem.a
    ${SOURCE_DIR}/lib/libboost_regex.a
    ${SOURCE_DIR}/lib/libboost_chrono.a
    ${SOURCE_DIR}/lib/libboost_atomic.a
    CACHE INTERNAL "")
