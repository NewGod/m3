#!/bin/bash

# connectNS [num1] [num2] [prefix] (assume num1 < num2)
connectNS()
{
    eval ./connectNS \$ns$1 \$ns$2 \$veth$1$2 \$veth$2$1 \$prefix$1$2
}

# connectDefNS [num1] [prefix] (assume num1 < num2)
#connectDefNS()
# {
#    eval ./connectDefNS \$ns$1 \$veth0$1 \$veth${1}0 \$prefix0$1
#}

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 5 ]; then
    echo "Usage: $0 [prefix12] [prefix23] [prefix34] [prefix03] [namePref]"
    exit 0
fi

if [ "s$5" = "s?" ]; then
    namePref=""
else
    namePref=$5
fi

ns0=${namePref}ns0
ns1=${namePref}ns1
ns2=${namePref}ns2
ns3=${namePref}ns3
ns4=${namePref}ns4
veth12=${namePref}veth12
veth21=${namePref}veth21
veth23=${namePref}veth23
veth32=${namePref}veth32
veth34=${namePref}veth34
veth43=${namePref}veth43
veth03=${namePref}veth03
veth30=${namePref}veth30
prefix03=$4
prefix12=$1
prefix23=$2
prefix34=$3

./addNS $ns1
./addNS $ns2
./addNS $ns3
./addNS $ns4

connectNS 1 2
connectNS 2 3
connectNS 3 4
connectNS 0 3
#connectDefNS 3

#./enableForward
./execNS $ns1 ./enableForward
./execNS $ns2 ./enableForward
./execNS $ns3 ./enableForward
./execNS $ns4 ./enableForward

./execNS $ns1 ./setGRoute default $veth12 $prefix12.2
./execNS $ns2 ./setGRoute default $veth23 $prefix23.2
./execNS $ns3 ./setGRoute default $veth30 $prefix03.1
./execNS $ns3 ./setGRoute $prefix12.0/24 $veth32 $prefix23.1
./execNS $ns4 ./setGRoute default $veth43 $prefix34.1
./setGRoute $prefix12.0/24 $veth03 $prefix03.2
./setGRoute $prefix23.0/24 $veth03 $prefix03.2
./setGRoute $prefix34.0/24 $veth03 $prefix03.2

#execNS $ns2 iptables -t nat -I POSTROUTING -o $veth23 -j MASQUERADE
#execNS $ns2 iptables -t nat -I PREROUTING -i $veth23 -d $prefix23.1 -j DNAT --to $prefix12.1