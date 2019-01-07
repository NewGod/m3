#!/bin/bash

version="0.5.1 Beta"
verbose=0
werror=0
debug=0
explicit=1
outpath="/tmp/m3-initdevice`date +%y%m%d-%H%M%S`.log"

getIP()
{
    toIP=`ip addr show $1 2> /dev/null | grep 'inet ' | awk '{print $2}'`
    if [ "s$ip" = "s" ]; then
        return 1
    else
        toIP=${toIP%/*}
        return 0
    fi
}

chooseEmptyTable()
{
    local file=/etc/iproute2/rt_tables
    local now=0
    local last=0
    for i in `cat $file | egrep ^[0-9] | awk {'print $1'} | sort -n`
    do
        now=$i
        if [ $now -eq 0 ]; then
            continue
        fi
        if [ $(( $now - $last )) -gt 1 ]; then
            tablenum=$(( $last + 1 ))
            return 0
        fi

        last=$now
    done

    return 1
}

vecho ()
{
if [ $verbose -eq 1 ]; then
    echo -e "\e[1;34m$*\e[0m"
fi
}

vecho2 ()
{
if [ $verbose -eq 1 ]; then
    echo "$*"
fi
}

fecho ()
{
echo "Fatal:" $@
clean
}

vechoAndExecute()
{
vecho2 "$*"
if [ "s$debug" = "s0" ]; then
    eval $@ >> "$outpath" 2>> "$outpath"
fi
return $?
}

doCommand()
{
    eval clean$1=0
    vechoAndExecute $3
    if [ $? -ne 0 ]; then
        fecho $2
        exit $1
    fi
    eval clean$1=1
}

doClean()
{
    eval local cleanflag=\$clean$1
    if [ "s$cleanflag" = "s1" ]; then
        vecho $2
        vecho2 $3
        if [ "s$debug" = "s0" ]; then
            eval $3 > /dev/null 2> /dev/null
        fi
    fi
}

clean()
{
vecho "Cleaning..."
doClean 21 "Clean routing table." "ip route flush table $device"
if [ "s$dopost" = "s1" ]; then
    doClean 18 "Remove routing policy." "ip rule del iif $to fwmark $mark table $device"
else
    doClean 20 "Remove routing policy." "ip rule del fwmark $mark table main"
    doClean 19 "Remove routing policy." "ip rule del iif $to table $device"
fi
doClean 17 "Remove routing table aliasing." "sed -i \"/$tablenum	$device/d\" /etc/iproute2/rt_tables"
if [ "s$dopost" = "s1" ]; then
    getIP $to
    doClean 14 "Remove MARK rules." "iptables -t mangle -D PREROUTING -p tcp -i $to --dport ${portrange/-/:} -j MARK --set-mark $mark"
    doClean 13 "Remove DNAT rule." "iptables -t nat -D PREROUTING -p tcp -d $toIP --dport ${portrange/-/:} -i $to -j DNAT --to $ip:$portrange"
    doClean 12 "Remove SNAT rule." "iptables -t nat -D POSTROUTING -p tcp -s $ip --sport ${portrange/-/:} -o $to -j MASQUERADE"
else
    doClean 15 "Remove MARK rule." "iptables -t mangle -D PREROUTING ! -p tcp -i $to -j MARK --set-mark $mark"
fi
doClean 5 "Remove devices." "ip link del $peer"
doClean 2 "Remove network namespace." "ip netns del $namespace"
}

echoHelp()
{
echo "initDevice from m3 utility $version"
echo "Usage: $0 [options]"
cat << EOF
    -A [deviceIP] (default 10.$mark.$mark.$mark):
    -a [peerIP] (default 10.$mark.$mark.234):
        Specify IP of veth device used for forwarding packets(peer device).
    -c:
        Clear the settings previously made by this script(you should specify same arguments specified for the setup work).
    -D [device] (default m3veth):
        Specify name of master devcie.
    -d [peerDevice] (default ${device}-peer):
        Specify name of peer device.
    -e:
        Accept default arguments. If not specified, program will report & exit
        when arguments are missing.
    -h(-?):
        Print this message.
    -m:
        Masquerade when sending packets to the device.
    -M [mark] (default $mark)
        Specify mark value used by mangle table.
    -n [namespace] (default m3ns):
        Specify name of the network namespace containing master device.
    -o [path] (default /dev/null):
        Specify file path to receive command output. 
    -p [portrange]:
        When masquerading, only forward port specified by this parameter.
    -t [device] (default lo):
        Specify name of the device that m3 program inject packet to.
    -v:
        Print verbose message.
EOF
}

# setDefault [arg] [default] [name] [retval] [optarg]
setDefault()
{
    eval local argval=\$$1

    if [ "s$argval" = "s" ]; then
        if [ "s$explicit" = "s1" ]; then
            echo "Fatal: Argument $3(specify with -$5) missing."
            exit $4
        fi
        eval $1=\$2
        eval vecho "Using default $3 \"\$$1\"."
    else
        vecho "Using $3 \"$argval\""
    fi
}

OPTIND=1

portrange="10-605"
dopost=0
while getopts "A:a:cD:d:eh?M:mn:o:p:t:v" opt; do
    case "$opt" in
    A)
        ip=$OPTARG
        ;;
    a)
        peerip=$OPTARG
        ;;
    c)
        doclean=1
        ;;
    D)
        device=$OPTARG
        ;;
    d)
        peer=$OPTARG
        ;;
    e)
        explicit=0
        ;;
    h|\?)
        echoHelp
        exit 0
        ;;
    m)
        dopost=1
        ;;
    M)
        mark=$OPTARG
        ;;
    n)
        namespace=$OPTARG
        ;;
    o)
        outpath=$OPTARG
        ;;
    p)
        portrange=$OPTARG
        ;;
    t)
        to=$OPTARG
        ;;
    v)
        verbose=1
        ;;
    esac
done

if [ $# -eq 0 ]; then
    echoHelp
    exit 0
fi

if [ `id -u` -ne 0 ]; then
    echo "Fatal: You need to be root to execute this."
    exit 1
fi

if [ $debug -eq 1 ]; then
    vecho "Debug mode"
fi

setDefault namespace "m3ns" "network namespace name" 1 n
setDefault device "m3veth" "device name" 1 D
setDefault peer "$device-peer" "peer device name" 1 d
setDefault ip 10.233.233.233 "IP" 1 A
setDefault peerip 10.233.233.234 "peer IP" 1 a
setDefault to "lo" "inject device name" 1 t
setDefault mark 233 "fwmark value" 1 MARK
if [ "s$doclean" = "s1" ]; then
    i=1
    while [ $i -lt 200 ];
    do
        eval clean$i=1
        let i=$i+1
    done
    clean
    exit 0
fi

vecho "Creating network namespace..."
if ls /var/run/netns 2> /dev/null | grep -q "$namespace"; then
    vecho "Network namespace \"$namespace\" exists."
else
    doCommand 2 "Cannot create network namespace." "ip netns add $namespace"
fi

vecho "Creating devices used by m3 program..."
if ip link list | grep -q "$device"; then
    fecho "Device \"$device\" exists."
    exit 3
fi
if ip link list | grep -q "$peer"; then
    fecho "Device \"$peer\" exists."
    exit 4
fi

doCommand 5 "Failed to add device." "ip link add $device type veth peer name $peer"

vecho "Setting up devices..."
doCommand 6 "Cannot add device \"$device\" to namespace \"$namespace\"." "ip link set $device netns $namespace"
doCommand 7 "Cannot activate device \"$device\"." "ip netns exec $namespace ip link set $device up"
doCommand 8 "Cannot activate device \"$peer\"." "ip link set $peer up"

vecho "Assigning IP for devices..."
doCommand 9 "Cannot assign IP \"$ip/24\" for device \"$device\"." "ip netns exec $namespace ip addr add $ip/24 dev $device"
doCommand 10 "Cannot assign IP \"$peerip/24\" for device \"$peer\"." "ip addr add $peerip/24 dev $peer"

vecho "Configuring routing table..."
if [ "s$dopost" = "s1" ]; then
    doCommand 11 "Cannot get IP for device \"$to\"." "getIP $to"
    doCommand 12 "Cannot set SNAT rules." "iptables -t nat -I POSTROUTING -p tcp -s $ip --sport ${portrange/-/:} -o $to -j MASQUERADE"
    doCommand 13 "Cannot set DNAT rules." "iptables -t nat -I PREROUTING -p tcp -d $toIP --dport ${portrange/-/:} -i $to -j DNAT --to $ip:$portrange"
    doCommand 14 "Cannot set MARK rules." "iptables -t mangle -I PREROUTING -p tcp -i $to --dport ${portrange/-/:} -j MARK --set-mark $mark"
else
    doCommand 15 "Cannot set MARK rules." "iptables -t mangle -I PREROUTING ! -p tcp -i $to -j MARK --set-mark $mark"
fi
doCommand 16 "No empty routing table left." "chooseEmptyTable"
doCommand 17 "Can't modify routing table list." "echo \"$tablenum	$device\" >> /etc/iproute2/rt_tables"
if [ "s$dopost" = "s1" ]; then
    doCommand 18 "Can't add routing policy." "ip rule add iif $to fwmark $mark table $device"
else
    doCommand 19 "Can't add routing policy." "ip rule add iif $to table $device"
    doCommand 20 "Can't add routing policy." "ip rule add fwmark $mark table main"
fi
doCommand 21 "Can't add routing rule" "ip route add default via $ip dev $peer table $device"
doCommand 22 "Can't add routing rule" "ip route add ${ip%.*}.0/24 dev $peer table $device"
doCommand 23 "Cannot enable IP forwarding." "sysctl -q net.ipv4.ip_forward=1"
doCommand 24 "Cannot enable IP forwarding." "sysctl -q net.ipv4.conf.all.forwarding=1"
doCommand 25 "Cannot set default routing rule in namespace $namespace" "ip netns exec $namespace ip route add default via $peerip"
doCommand 26 "Cannot set filtering rule in namespace $namespace" "ip netns exec $namespace iptables -I INPUT -j DROP"
doCommand 27 "Cannot disable TSO on \"$to\"" "ethtool -K $to tx off sg off tso off"

echo "Completed."
exit 0
