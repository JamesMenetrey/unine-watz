#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/polybench/

wasm_heap_size=$((10 * 1024 * 1024))
polybench_files=$SCRIPT_DIR/out/native/*


for file in $polybench_files
do
    local_file=$(basename $file)
    announcerun "polybench REE ($local_file)"

    for iterations in {1..50}
    do
        echo "$local_file,$(sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "polybench-native/$local_file 2>&1")" | tee -a $LOGS_DIR/polybench/ree-native.csv
    done
done
