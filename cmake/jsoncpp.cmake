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

ExternalProject_Add(jsoncpp
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp"
    URL https://github.com/open-source-parsers/jsoncpp/archive/1.8.4.tar.gz
    URL_HASH SHA256=c49deac9e0933bcb7044f08516861a2d560988540b23de2ac1ad443b219afdb6
    TIMEOUT 30
    INSTALL_COMMAND ""
    CMAKE_ARGS -DJSONCPP_WITH_PKGCONFIG_SUPPORT=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -DJSONCPP_WITH_TESTS=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    DOWNLOAD_NO_PROGRESS true
    )

set_property(DIRECTORY PROPERTY CLEAN_NO_CUSTOM
    "${CMAKE_CURRENT_BINARY_DIR}/jsoncpp")

ExternalProject_Get_Property(jsoncpp source_dir)
set(JSONCPP_INCLUDE_DIRS ${source_dir}/include)
include_directories(${JSONCPP_INCLUDE_DIRS})

ExternalProject_Get_Property(jsoncpp binary_dir)
link_directories(${binary_dir}/src/lib_json/)

set(JSONCPP_LIBRARIES ${binary_dir}/src/lib_json/libjsoncpp.a)
