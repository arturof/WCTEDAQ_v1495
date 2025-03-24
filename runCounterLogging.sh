#!/bin/bash

cd /home/mpmt/Monitoring/libDAQInterface
source Setup.sh
cd /home/mpmt/firmware/v1495
json_config=/home/mpmt/firmware/TriggerConfig/configurations/written_config.json
./v1495-counters-database -l a4818 -a 25501 -c 0 -v 3210 -j ${json_config} -r >& /dev/null &
