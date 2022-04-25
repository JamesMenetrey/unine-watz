#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

deploywatz

announcedeploy "launch-time"
cd $SCRIPT_DIR
rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/launch-time/out/\*.aot out/
sshpass -p "$BM_BOARD_USER" rsync --progress -r out/ $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/root/launch-time