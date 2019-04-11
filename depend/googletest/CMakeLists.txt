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

set(SOURCE_DIR ${CMAKE_BINARY_DIR}/googletest/src/googletest)

set(GTEST_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "")
set(GMOCK_INCLUDE_DIR ${SOURCE_DIR}/include CACHE INTERNAL "")

set(GMOCK_BOTH_LIBRARIES
    ${SOURCE_DIR}/lib/libgmock_main.a
    ${SOURCE_DIR}/lib/libgmock.a
    CACHE INTERNAL "")

set(GTEST_BOTH_LIBRARIES
    ${SOURCE_DIR}/lib/libgtest_main.a
    ${SOURCE_DIR}/lib/libgtest.a
    CACHE INTERNAL "")
