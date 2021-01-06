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
original_ot_arq=0
type="idle"

# cmd argument parsing
while getopts t:s:g:r:l:n:dco flag
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
            o) original_ot_arq=1;;
        esac
    done

echo "symbol_size: $symbol_size"
echo "gen_size: $gen_size"
echo "redundancy: $redundancy"
echo "log_file_name: $log_file_name"
echo "number_loops: $number_loops"

# NC
if [[ $type = "nc" ]]
then
    for (( i=0; i<$number_loops; i++ ))
    do
        echo "round $i"
        # SNC
        if [[ $density = 1 ]]
        then
            sudo ../build/wireless_nc_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_server.dump -d &
            sudo ../build/wireless_nc_relay_smart -p /dev/ttyACM1 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_relay.dump -d &
            sudo ../build/wireless_nc_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -r $redundancy -d &>/dev/null
            sleep 1.2s
        # RNC
        elif [[ $recode = 1 ]]
        then
            sudo ../build/wireless_nc_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_server.dump -c &
            sudo ../build/wireless_nc_relay_smart -p /dev/ttyACM1 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_relay.dump -c &
            sudo ../build/wireless_nc_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -r $redundancy -c  &>/dev/null
            sleep 1.2s
        # NC
        else
            sudo ../build/wireless_nc_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_server.dump &
            sudo ../build/wireless_nc_relay_smart -p /dev/ttyACM1 -s $symbol_size -g $gen_size -r $redundancy -l ${log_file_name}_nc_relay.dump &
            sudo ../build/wireless_nc_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -r $redundancy  &>/dev/null
            sleep 1.2s
        fi
        sudo killall -9  wireless_nc_server
        sudo killall -9  wireless_nc_relay_smart
        sudo killall -9  wireless_nc_client
    done
# OT ARQ
elif [[ $type = "no" ]]
then
    # original OT ARQ
    if [[ $original_ot_arq = 1 ]]
    then
        for (( i=0; i<$number_loops; i++ ))
        do
            echo "round $i"
            sudo ../build/wireless_no_coding_relay -p /dev/ttyACM1 -s $symbol_size -g $gen_size -l ${log_file_name}_no_relay.dump &
            sudo ../build/wireless_no_coding_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -l ${log_file_name}_no_dst.dump &
            sudo ../build/wireless_no_coding_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -l ${log_file_name}_no_src.dump &
            sleep 2.3s
            sudo killall -9  wireless_no_coding_relay
            sudo killall -9  wireless_no_coding_server
            sudo killall -9  wireless_no_coding_client
            sleep 0.2s
        done
    else
        # smart OT ARQ
        for (( i=0; i<$number_loops; i++ ))
        do
            echo "round $i"
            sudo ../build/wireless_no_coding_relay_smart -p /dev/ttyACM1 -s $symbol_size -g $gen_size -l ${log_file_name}_no_relay_smart.dump &
            sudo ../build/wireless_no_coding_server -p /dev/ttyACM2 -s $symbol_size -g $gen_size -l ${log_file_name}_no_dst_smart.dump &
            sudo ../build/wireless_no_coding_client -p /dev/ttyACM0 -s $symbol_size -g $gen_size -l ${log_file_name}_no_src_smart.dump &
            sleep 2.3s
            sudo killall -9  wiireless_no_coding_relay_smart
            sudo killall -9  wireless_no_coding_server
            sudo killall -9  wireless_no_coding_client
            sleep 0.2s
        done
    fi
else
    echo "wrong type!"
fi
