#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

deploywatz
deploywamr

announcedeploy "polybench"
cd $SCRIPT_DIR
mkdir -p out
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/polybench/out/\*.aot out/wasm
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/dist/polybench-c/output/\* out/native
sshpass -p "$BM_BOARD_USER" rsync --progress -r out/wasm/ $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root/polybench-wasm/
sshpass -p "$BM_BOARD_USER" rsync --progress -r out/native/ $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root/polybench-native/
