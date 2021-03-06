#!/bin/bash

handler()
{
	cd "$ROOT/.."
	echo "Caught a signal, exiting..."
	exit 0
}

run_one()
{
	local script="`basename "$1"`"
	local tdir="$TESTDIR/$script"
	mkdir -p $tdir
	[ $? -ne 0 ] && return 1

	pushd $tdir >/dev/null
	ln -s "$ROOT/$script" $script

	./$script > run.log 2>&1
	local rc=$?

	popd >/dev/null
	return $rc
}

TESTS=`find tests/ -type f -executable | sort`

ELFREF=`pwd`/elfref
[ ! -x $ELFREF ] && echo "$ELFREF not executable, exiting..." && exit 1
echo "Will run tests for $ELFREF"
export ELFREF

export ROOT="`pwd`/tests/"

umask 00002

trap "handler()" 2

TESTDIR="RUN.$$"
[ -d "$TESTDIR" ] && echo "Testrun directory $TESTDIR not empty, exiting..." && exit 1

echo "Creating testrun directory $TESTDIR..."
mkdir "$TESTDIR"


let good=0
let bad=0
for T in $TESTS; do
	TNAME="$TESTDIR/`basename $T`"
	echo -n "Running	$TNAME		"
	run_one $T
	if [ $? -ne 0 ]; then
		echo "FAILED"
		let bad++
	else
		echo "SUCCESS"
		let good++
	fi
done

echo
echo "Done running tests in $TESTDIR"
echo "Success: $good"
echo "Failed: $bad"

