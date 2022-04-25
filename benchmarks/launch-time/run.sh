#!/bin/bash

# Warm up:
# vedliot_attester 12582912 launch-time/bm_launch-time_11.aot

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcerun "launch-time"

mkdir -p $LOGS_DIR/launch-time/
rm -f $LOGS_DIR/launch-time/general.csv

for size in {1..9}
do
    for iterations in {1..100}
    do
        wasm_heap_size=$((($size + 4) * 1024 * 1024))

        echo "$size,$(sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size launch-time/bm_launch-time_$size.aot 2>&1")" | tee -a $LOGS_DIR/launch-time/general.csv
        safesleep
    done
done