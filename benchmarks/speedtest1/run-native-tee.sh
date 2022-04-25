#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/speedtest1

announcerun "SQLite (native TEE)"

for iterations in {1..50}
do
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME '/root/bm_speedtest1_native_tee 2>&1' | tee -a $LOGS_DIR/speedtest1/tee-native.csv
done
