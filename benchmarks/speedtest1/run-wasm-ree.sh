#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/speedtest1/

wasm_heap_size=$((20 * 1024 * 1024))
wasm_stack_size=$((512 * 1024))

announcerun "SQLite (Wasm REE)"

for iterations in {1..50}
do
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "iwasm --global-heap-size=$wasm_heap_size --stack-size=$wasm_stack_size bm_speedtest1.aot 2>&1" | tee -a $LOGS_DIR/speedtest1/ree-wasm.csv
done
