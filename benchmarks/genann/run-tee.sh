#!/bin/bash

# Warm up:
# vedliot_verifier 100
# vedliot_attester 12582912 genann/bm_genann_tee.aot

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/genann/

announcerun "Genann TEE"
wasm_heap_size=$((12 * 1024 * 1024))
wasm_stack_size=$((512 * 1024))

# Prototype: launch_verifier <number_of_bytes_of_data>
launch_verifier () {
    size_of_dataset=$1

    sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_verifier $size_of_dataset 2>> /root/genann-error-verifier.log" | tee -a $LOGS_DIR/genann/verifier.csv
}

for size in {100..1000..100}
do
    for iterations in {1..20}
    do
        restart_tee_supplicant

        launch_verifier $size &
        safesleep
        echo "$size,$(sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size genann/bm_genann_tee.aot 2>&1")" | tee -a $LOGS_DIR/genann/tee.csv
        safesleep
    done
done
