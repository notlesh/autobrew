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

function p2_on () {
	curl http://localhost/ab?cmd=p2_on
}

function p2_off () {
	curl http://localhost/ab?cmd=p2_off
}

p1_off
p2_off

while true; do
	sleep 5
	p1_on
	sleep 60
	p1_off
	sleep 5
	p2_on
	sleep 60
	p2_off
done
