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

include(GoogleTest)

function(add_gmock_test target)
    set(target ${target}_tests)
    add_executable(${target} ${test_srcs})
    add_dependencies(${target} boost googletest jsoncpp ${test_deps})
    target_link_libraries(${target} ${test_libs} ${GMOCK_BOTH_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES} ${test_link} pthread)
    target_include_directories(${target} PRIVATE ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ${JSONCPP_INCLUDE_DIRS} ${ROCKSDB_INCLUDE_DIRS})
    gtest_discover_tests(${target})
    unset(test_srcs)
    unset(test_libs)
    unset(test_deps)
    unset(test_link)
endfunction()
