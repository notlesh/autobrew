#!/bin/bash

# this script will heat the HLT for a bit, then the BK. it will then alternate
# between the two. run this an hour or two before desired mash in and both
# should be heated (use the "at" command)

# requirements / checklist
# HLT should be full of water, at least above heating element / temp probe
# BK should be full of water, at least above heating element / temp probe
# BK should be set up to whirlpool water with pump 2 (!!!)

if [ "$EUID" -ne 0 ]; then
	echo "****** Run as root ******"
	exit
fi


bk_temp=`~pi/ab2/tools/other/ftc.sh 163`
hlt_temp=`~pi/ab2/tools/other/ftc.sh 160`

function fireProbes() {

# first fire HLT
gpio export 18 out; gpio -g write 18 1
timeout 20m ~pi/ab2/tools/pid_temp_controller/debug/pid_temp_controller --temp-probe 28.EE9B8B040000 --pin-id 17 --safety-id 10 -s $bk_temp
gpio export 18 out; gpio -g write 18 0

# now turn on pump 2, fire BK, then turn off pump 2
timeout 30m ~pi/ab2/tools/pid_temp_controller/debug/pid_temp_controller --temp-probe 28.3AA87D040000 --pin-id 4 --safety-id 24 -s $hlt_temp

sleep 10

}

while true; do 
	fireProbes;
done
