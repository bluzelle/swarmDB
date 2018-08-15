#!/usr/bin/env bash

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
