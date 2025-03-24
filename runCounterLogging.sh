#!/bin/bash

cd /home/mpmt/Monitoring/libDAQInterface
source Setup.sh
cd /home/mpmt/firmware/v1495
./v1495-counters-database -l a4818 -a 25501 -c 0 -v 3210 -r >& /dev/null &
