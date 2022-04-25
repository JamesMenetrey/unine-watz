#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

mkdir -p $LOGS_DIR/speedtest1/

announcerun "SQLite (Native REE)"

for iterations in {1..50}
do
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "/root/bm_speedtest1_native_ree 2>&1" | tee -a $LOGS_DIR/speedtest1/ree-native.csv
done
