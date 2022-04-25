#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/speedtest1/

wasm_heap_size=$((21 * 1024 * 1024))

for iterations in {1..50}
do
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size bm_speedtest1.aot 2>&1" | tee -a $LOGS_DIR/speedtest1/tee-wasm.csv
done
