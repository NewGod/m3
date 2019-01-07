#!/bin/bash

execNS() {
    ip netns exec $*
}

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 [namePref]"
    exit 0
fi

if [ "s$1" = "s?" ]; then
    namePref=""
else
    namePref=$1
fi

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

./delVeth $veth03
execNS $ns1 ./delVeth $veth12
execNS $ns2 ./delVeth $veth23
execNS $ns3 ./delVeth $veth34
./delNS $ns1
./delNS $ns2
./delNS $ns3
./delNS $ns4
