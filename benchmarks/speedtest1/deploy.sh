#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

deploywatz
deploywamr

announcedeploy "SQLite"

cd $SCRIPT_DIR

rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/speedtest1/out/\* out/
sshpass -p "$BM_BOARD_USER" rsync --progress out/bm_speedtest1.aot $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root
sshpass -p "$BM_BOARD_USER" rsync --progress out/bm_speedtest1_native_ree $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root

sshpass -p "$BM_BOARD_USER" rsync --progress out/ca/bm_speedtest1_native_tee $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root
sshpass -p "$BM_BOARD_USER" rsync --progress out/ta/b4c9d236-cb68-45a5-b218-4266ca43b237.ta $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/lib/optee_armtz
sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME 'chmod 666 /lib/optee_armtz/b4c9d236-cb68-45a5-b218-4266ca43b237.ta'
