#!/bin/bash

# connectNS [num1] [num2] [prefix] (assume num1 < num2)
connectNS()
{
    eval ./connectNS \$ns$1 \$ns$2 \$veth$1$2 \$veth$2$1 \$prefix$1$2
}

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 6 ]; then
    echo "Usage: $0 [prefix01] [prefix12] [portrange] [indevice] [indeviceip] [namePref]"
    exit
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

./addNS $ns1
./addNS $ns2

connectNS 0 1
connectNS 1 2

./enableForward
./execNS $ns1 ./enableForward

./execNS $ns1 ./setGRoute default $veth10 $prefix01.1
./execNS $ns2 ./setGRoute default $veth21 $prefix12.1

iptables -t nat -I POSTROUTING -o $4 -j MASQUERADE
iptables -t nat -I PREROUTING -p tcp -i $4 -d $5 --dport ${3/-/:} -j DNAT --to $prefix01.2
