#!/bin/bash
apt update
apt install -y gcc g++ git gdb make libhdf5-serial-dev libpcap-dev libnet1-dev xutils-dev ethtool vim net-tools gnuplot bison flex
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON/
make
make install
cd ..
rm -rf cJSON
