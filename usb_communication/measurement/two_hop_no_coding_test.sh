#!/bin/bash

set -uo pipefail

# default configuration
symbol_size=4
gen_size=10
redundancy=20
log_file_name="log"
number_loops=10
density=0
recode=0
type="idle"

# cmd argument parsing
while getopts t:s:g:r:l:n:dc flag
    do
        case "${flag}" in
            t) type=${OPTARG};;
            s) symbol_size=${OPTARG};;
            g) gen_size=${OPTARG};;
            r) redundancy=${OPTARG};;
            l) log_file_name=${OPTARG};;
            n) number_loops=${OPTARG};;
            d) density=1;;
            c) recode=1;;
        esac
    done

echo "symbol_size: $symbol_size"
echo "gen_size: $gen_size"
echo "redundancy: $redundancy"
echo "log_file_name: $log_file_name"
echo "number_loops: $number_loops"

if [[ $type = "no" ]]
then
    for (( i=0; i<$number_loops; i++ ))
    do
        echo "round $i"
        sudo ../build/wireless_no_coding_relay_smart -p /dev/ttyACM1 -s $symbol_size -g $gen_size -l ${log_file_name}_no_relay.dump &
        sudo ../build/wireless_no_coding_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -l ${log_file_name}_no_dst.dump &
        sudo ../build/wireless_no_coding_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -l ${log_file_name}_no_src.dump &
        sleep 2.3s
        sudo killall -9  wireless_no_coding_relay
        sudo killall -9  wireless_no_coding_server
        sudo killall -9  wireless_no_coding_client
        sleep 0.2s
    done
else
    echo "wrong type!"
fi
