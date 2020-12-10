#!/bin/bash

set -uo pipefail

# default configuration
symbol_size=4
gen_size=10
redundancy=20
log_file_name="log.dump"
number_loops=10
type="idle"

# cmd argument parsing
while getopts t:s:g:r:l:n: flag
    do
        case "${flag}" in
            t) type=${OPTARG};;
            s) symbol_size=${OPTARG};;
            g) gen_size=${OPTARG};;
            r) redundancy=${OPTARG};;
            l) log_file_name=${OPTARG};;
            n) number_loops=${OPTARG};;
        esac
    done

echo "symbol_size: $symbol_size"
echo "gen_size: $gen_size"
echo "redundancy: $redundancy"
echo "log_file_name: $log_file_name"
echo "number_loops: $number_loops"

if [[ $type = "nc" ]]
then
for (( i=0; i<$number_loops; i++ ))
do
    echo "round $i"
    sudo ../build/wireless_nc_server -p /dev/ttyACM1 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}.dump &
    sudo ../build/wireless_nc_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -r $redundancy  &>/dev/null
    sleep 1.2s
    sudo killall -9  wireless_nc_server
    sudo killall -9  wireless_nc_client
done
elif [[ $type = "no" ]]
then
for (( i=0; i<$number_loops; i++ ))
do
    echo "round $i"
    sudo ../build/wireless_no_coding_server -p /dev/ttyACM1 -s $symbol_size -g $gen_size -l ${log_file_name}_no_dst.dump &
    sudo ../build/wireless_no_coding_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -l ${log_file_name}_no_src.dump &>/dev/null
    sleep 1.5s
    sudo killall -9  wireless_no_coding_server
    sudo killall -9  wireless_no_coding_client

done
else
    echo "wrong type!"
fi
