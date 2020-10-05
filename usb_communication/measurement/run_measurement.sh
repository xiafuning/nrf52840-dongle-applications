#!/bin/bash

set -euo pipefail

# default configuration
type="idle"
log_file_name="log.dump"
number_runs=10
rounds_per_run=20

# cmd argument parsing
while getopts t:l:n:r: flag
    do
        case "${flag}" in
            t) type=${OPTARG};;
            l) log_file_name=${OPTARG};;
            n) number_runs=${OPTARG};;
            r) rounds_per_run=${OPTARG};;
        esac
    done

echo "log_file_name: $log_file_name"
echo "number_runs: $number_runs"
echo "rounds_per_run: $rounds_per_run"


if [[ $type = "one" ]]
then
    for (( i=0; i<$number_runs; i++ ))
    do
        ./one_hop_measurement.sh -s 256 -g 1 -l ${log_file_name} -n ${rounds_per_run} -t no
        sleep 2s
        ./one_hop_measurement.sh -s 64 -g 4 -r 0 -l ${log_file_name} -n ${rounds_per_run} -t nc
        sleep 2s
        ./one_hop_measurement.sh -s 64 -g 4 -r 50 -l ${log_file_name} -n ${rounds_per_run} -t nc
        sleep 2s
        ./one_hop_measurement.sh -s 64 -g 4 -r 75 -l ${log_file_name} -n ${rounds_per_run} -t nc
        sleep 2s
        ./one_hop_measurement.sh -s 64 -g 4 -r 100 -l ${log_file_name} -n ${rounds_per_run} -t nc
        sleep 5s
    done
elif [[ $type = "two" ]]
then
    echo "to be implemented!"
else
    echo "wrong type!"
fi
