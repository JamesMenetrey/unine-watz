#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/genann/

wasm_heap_size=$((12 * 1024 * 1024))
wasm_stack_size=$((512 * 1024))

announcerun "Genann REE"

for size in {100..1000..100}
do
    for iterations in {1..20}
    do
        echo "$size,$(sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "iwasm --global-heap-size=$wasm_heap_size --stack-size=$wasm_stack_size --dir=. genann/bm_genann_ree.aot $size 2>&1")" | tee -a $LOGS_DIR/genann/ree.csv
    done
done
