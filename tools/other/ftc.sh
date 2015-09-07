#!/bin/bash

if [ ! -n "$1" ]; then
	echo "require argument: temperature in fahrenheit to be converted to celcius";
	exit 1
fi

bc_math="scale=8;($1 - 32.0) * (5/9)"
# echo "math: $bc_math"
result=`echo "$bc_math" | bc -l`
echo $result
