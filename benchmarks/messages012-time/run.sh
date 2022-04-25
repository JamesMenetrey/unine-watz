#!/bin/bash

# Warm up:
# vedliot_verifier 256
# vedliot_attester 2097152 bm_messages012.aot

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/messages012-time/

announcerun "messages012-time"
data_size=256
wasm_heap_size=$((2 * 1024 * 1024))

launch_verifier () {
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_verifier $data_size 2>&1" | tee -a $LOGS_DIR/messages012-time/messages012-verifier.csv
}

for iterations in {1..100}
do
    launch_verifier &
    safesleep
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size bm_messages012.aot 2>&1" | tee -a $LOGS_DIR/messages012-time/messages012-attester.csv
    safesleep
done
