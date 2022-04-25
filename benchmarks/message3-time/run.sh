#!/bin/bash

# Warm up:
# vedliot_verifier 512
# vedliot_attester 4194304 message3-time/bm_message3_512.aot

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/message3-time/

announcerun "message3-time"
wasm_heap_size=$((4 * 1024 * 1024))

# Prototype: launch_verifier <number_of_bytes_of_data>
launch_verifier () {
    number_of_bytes_of_data=$1

    echo "$KB,$(sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_verifier $number_of_bytes_of_data 2>> /root/error-verifier.log")" | tee -a $LOGS_DIR/message3-time/verifier.csv
}

for KB in {512..3072..512}
do
    bytes=$(($KB * 1024))

    for iterations in {1..100}
    do
        restart_tee_supplicant
        launch_verifier $bytes &
        safesleep
        echo "$KB,$(sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "vedliot_attester $wasm_heap_size message3-time/bm_message3_$KB.aot")" | tee -a $LOGS_DIR/message3-time/attester.csv
        safesleep
    done
done
