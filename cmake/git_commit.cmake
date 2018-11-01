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

set(SWARM_GIT_COMMIT "unknown")

find_program(GIT_EXECUTABLE NAMES git)

if (GIT_EXECUTABLE)
    execute_process(COMMAND git describe --always --tags --dirty OUTPUT_VARIABLE SWARM_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/swarm_git_commit.hpp.in ${PROJECT_BINARY_DIR}/swarm_git_commit.hpp.tmp)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_BINARY_DIR}/swarm_git_commit.hpp.tmp ${PROJECT_BINARY_DIR}/swarm_git_commit.hpp)
execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_BINARY_DIR}/swarm_git_commit.hpp.tmp)
