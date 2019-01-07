#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 6 ]; then
    echo "Usage $0 [prefix01] [prefix12] [portrange] [indevice] [indeviceip] [namePref]"
fi

if [ "s$6" = "s?" ]; then
    namePref=""
else
    namePref=$6
fi

ns0=${namePref}ns0
ns1=${namePref}ns1
ns2=${namePref}ns2
veth01=${namePref}veth01
veth10=${namePref}veth10
veth12=${namePref}veth12
veth21=${namePref}veth21
prefix01=$1
prefix12=$2

iptables -t nat -D POSTROUTING -o $4 -j MASQUERADE
iptables -t nat -D PREROUTING -p tcp -i $4 -d $5 --dport ${3/-/:} -j DNAT --to $prefix01.2

./execNS $ns1 ./delVeth $veth10
./execNS $ns1 ./delVeth $veth12
./delNS $ns2
./delNS $ns1
