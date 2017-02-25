#!/bin/bash

# script to cycle between pump 1 and pump 2. designed to pump back-and-forth
# to clean plate chiller.

# hook up as follows:
# bk-out -> pump 1 in
# pump 1 out -> pump 2 out
# pump 2 in -> chiller wort in
# chiller wort out -> bk-in

function p1_on () {
	curl http://localhost/ab?cmd=p1_on
}

function p1_off () {
	curl http://localhost/ab?cmd=p1_off
}

function printHelp () {
	echo "mash_pump_cycle.h"
	echo "  cycles pump 3 on and off."
	echo "  usage:"
	echo "      mash_pump_cycle.h <seconds on> <seconds off>"
}

# make sure we have 2 args or more
if [ $# -ne 2 ]; then
	printHelp
	exit 1
fi

echo "pump 1 cycling. on: $1, off: $2"

while true; do
	p1_on
	sleep $1
	p1_off
	sleep $2
done
