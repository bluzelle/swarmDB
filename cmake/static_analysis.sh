#!/usr/bin/env bash

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

# set -x

TEMP_FILE=$(mktemp)

case "$1" in
    cpd)
        find "$3" -not \( -path "$4" -prune \) -name "*.cpp" -not -name "*_test.cpp" -o -name "*.hpp" > $TEMP_FILE
        $2 cpd --minimum-tokens 115 --language cpp --filelist $TEMP_FILE
    ;;
    pmccabe)
        echo "Modified McCabe Cyclomatic Complexity"
        echo "|   Traditional McCabe Cyclomatic Complexity"
        echo "|       |    # Statements in function"
        echo "|       |        |   First line of function"
        echo "|       |        |       |   # lines in function"
        echo "|       |        |       |       |  filename(definition line number):function"
        echo "|       |        |       |       |           |"
        pmccabe $(find "$2" -not \( -path "$3" -prune \) -name "*.cpp" -not -name "*_test.cpp" -o -name "*.hpp") 2> $TEMP_FILE | sort -nr
        echo "-----------------------------------------------------------------------------"
        echo Parse errors:
        echo
        cat $TEMP_FILE
    ;;
esac

rm $TEMP_FILE
