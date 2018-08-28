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

add_custom_target(static_analysis)

# copy paste detection...
if (NOT DEFINED "PMD_EXE")
    find_program(PMD_EXE NAMES pmd run.sh)
endif()
message(STATUS "pmd: ${PMD_EXE}")

if (PMD_EXE)
    add_custom_target(cpd COMMAND ${CMAKE_SOURCE_DIR}/cmake/static_analysis.sh cpd ${PMD_EXE} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    add_dependencies(static_analysis cpd)
endif()

# complexity analysis...
find_program(PMCCABE_EXE NAMES pmccabe)
message(STATUS "pmccabe: ${PMCCABE_EXE}")

if (PMCCABE_EXE)
    add_custom_target(pmccabe COMMAND ${CMAKE_SOURCE_DIR}/cmake/static_analysis.sh pmccabe ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})
    add_dependencies(static_analysis pmccabe)
endif()
