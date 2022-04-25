#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../../common.sh

deploywatz
deploywamr

announcedeploy "latencies-wasm"
cd $SCRIPT_DIR
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/latencies/wasm/out/bm_latencies.aot out/
sshpass -p "$BM_BOARD_USER" rsync --progress out/bm_latencies.aot $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root
