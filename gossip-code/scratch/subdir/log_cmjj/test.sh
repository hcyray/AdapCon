#!/bin/bash
x=0
for i in {0..14}
	do
	y=$[i+1]
	for j in $(seq $y 15)
		do
		file1='node_'$i'_receiving_time_log.txt'
		file2='node_'$j'_receiving_time_log.txt'
		diff $file1 $file2
		x=$?
		if [ $x -ne 0 ]; then
			echo " "$i", "$j" has difference"
		fi	
		done
	done
