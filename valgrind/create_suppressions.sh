#!/bin/bash

# $ cd <build_dir>/output
# $ <source_dir>/valgrind/create_suppressions.sh
# $ cp bluzelle.supp <source_dir>/valgrind/

echo "Clearing stale valgrind logs & suppression files..."

rm -f *_tests_valgrind.log *.supp
rm -f bluzelle.supp

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

for utest in *_tests
do
	if valgrind --leak-check=full --show-reachable=yes --error-limit=no --gen-suppressions=all --log-file=${utest}_valgrind.log ./$utest; then
		echo "Creating suppression file for ${utest}..."
		cat ./${utest}_valgrind.log | $SCRIPT_DIR/parse_valgrind_suppressions.sh > ./${utest}.supp
		cat ./${utest}.supp >> bluzelle.supp
	else
		echo "${utest} exited with an error!!!"
		exit 1
	fi
done

echo "Done!"
