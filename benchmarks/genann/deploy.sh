#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

deploywatz
deploywamr

announcedeploy "Genann"
cd $SCRIPT_DIR
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/dist/webassembly-genann/example/iris/\* out/
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/genann/out/\*.aot out/
sshpass -p "$BM_BOARD_USER" rsync --progress -r out/ $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root/genann
