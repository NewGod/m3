#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 5 ]; then
    echo "Usage: $0 [nsnum1] [nsnum2] [num] [prefix] [namePref]"
    exit 0
fi

if [ "s$5" = "s?" ]; then
    namePref=""
else
    namePref=$5
fi

ns1=${namePref}ns$1
ns2=${namePref}ns$2
veth12=${namePref}veth$1$2
veth21=${namePref}veth$2$1

i=0
while [ $i -ne $3 ]; 
do
    ./connectNS $ns1 $ns2 ${veth12}-$i ${veth21}-$i $4.$i
    ./execNS $ns1 ip route del $4.$i.0/24 2> /dev/null
    ./execNS $ns1 ./multiGateway ${veth12}-$i default $4.$i.1 $4.$i.2
    
    i=$(($i + 1))
done