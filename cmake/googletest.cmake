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

ExternalProject_Add(googletest
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/googletest"
    URL https://github.com/google/googletest/archive/release-1.8.0.tar.gz
    URL_HASH SHA256=58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8
    TIMEOUT 30
    INSTALL_COMMAND ""
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/googletest")

ExternalProject_Get_Property(googletest source_dir)
include_directories(${source_dir}/googlemock/include ${source_dir}/googletest/include)

ExternalProject_Get_Property(googletest binary_dir)
link_directories(${binary_dir}/googlemock)

set(GMOCK_BOTH_LIBRARIES gmock_main gmock)
