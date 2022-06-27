#!/bin/sh

nodes=`seq 17 121`

lives=''
deads=''

for i in $nodes
do
	#echo -n 192.168.10.$i
	ret=`ping -c 1 192.168.10.$i`
	if test "$?" -eq 0
	then
		#if test `expr $i % 10` -eq 0
		#then
		#	echo -n -e '\r            \r'
		#fi
		#echo -n '.'
		echo -n -e '\r' $i '  '
		lives=$lives' '$i		
	else
		echo -e '\r'
		echo 'No responce 192.168.10.'$i :  $?
		deads=$deads' '$i
	fi
done

echo
echo 'Lives: ' $lives
echo 'Deads: ' $deads
