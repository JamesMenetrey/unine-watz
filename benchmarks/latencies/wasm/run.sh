#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../../common.sh

wasm_heap_size=$((1 * 1024 * 1024))

announcerun "latencies-wasm (time in WAMR)"
sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME 'iwasm bm_latencies.aot 1000 2>&1' | tee $LOGS_DIR/latencies/wasm-catime.csv

announcerun "latencies-wasm (time in WaTZ)"
sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size bm_latencies.aot 1000 2>&1" | tee $LOGS_DIR/latencies/wasm-tatime.csv
