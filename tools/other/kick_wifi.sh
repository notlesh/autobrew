#!/bin/bash

ping -c 1 -w 1 192.168.10.1
if [[ $? != 0 ]]; then
	# echo "Can't ping! Rebooting. `date`" >> ~pi/ping_fail_log
	# reboot

	echo "Can't ping! Reloading wifi kernel module... `date`" >> ~pi/ping_fail_log
	modprobe -vr 8192cu && modprobe -v 8192cu
fi
