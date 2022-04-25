#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

deploywatz

announcedeploy "messages012-time"
cd $SCRIPT_DIR
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/messages012-time/out/bm_messages012.aot out/
sshpass -p "$BM_BOARD_USER" rsync --progress out/bm_messages012.aot $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root
