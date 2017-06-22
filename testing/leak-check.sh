#!/bin/bash
#
# Script to run valgrind against the test suite for hunting memory leaks
#
# This assumes you run it in the Hammer base directory and have a debug build

HAMMER_ROOT=.
VARIANT=debug
BUILD_PATH=$HAMMER_ROOT/build/$VARIANT
LD_LIBRARY_PATH=$BUILD_PATH/src:$LD_LIBRARY_PATH
VALGRIND=valgrind
VALGRIND_OPTS="-v --leak-check=full --leak-resolution=high --num-callers=40 --partial-loads-ok=no --show-leak-kinds=all --track-origins=yes --undef-value-errors=yes"
VALGRIND_SUPPRESSIONS="valgrind-glib.supp"

for s in $VALGRIND_SUPPRESSIONS
do
  VALGRIND_OPTS="$VALGRIND_OPTS --suppressions=$HAMMER_ROOT/testing/valgrind/$s"
done

export LD_LIBRARY_PATH

$VALGRIND $VALGRIND_OPTS $BUILD_PATH/src/test_suite $@
