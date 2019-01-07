#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 4 ]; then
    echo "Usage: $0 [nsnum1] [nsnum2] [num] [namePref]"
    exit 0
fi

if [ "s$4" = "s?" ]; then
    namePref=""
else
    namePref=$4
fi

ns1=${namePref}ns$1
ns2=${namePref}ns$2
veth12=${namePref}veth$1$2
veth21=${namePref}veth$2$1

i=0
while [ $i -ne $3 ]; 
do
    ./execNS $ns1 ./delVeth ${veth12}-$i
    ./execNS $ns1 ./removeMultiGateway ${veth12}-$i

    i=$(($i + 1))
done