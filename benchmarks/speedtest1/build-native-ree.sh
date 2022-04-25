#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "SQLite (Native REE)"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

/opt/watz/toolchains/aarch64/bin/aarch64-linux-gnu-gcc $BM_CFLAGS \
    -DSQLITE_OS_OTHER \
    -DSQLITE_ENABLE_MEMSYS3 \
    -I$DIST_DIR/sqlite \
    -o $SCRIPT_DIR/out/bm_speedtest1_native_ree $SCRIPT_DIR/app/speedtest1.c $DIST_DIR/sqlite/*.c
