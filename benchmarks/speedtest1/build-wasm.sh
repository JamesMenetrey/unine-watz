#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "SQLite (Wasm)"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

/opt/wasi-sdk/bin/clang $BM_CFLAGS \
    --target=wasm32-wasi \
    --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
    -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
    -Wl,--strip-all \
    -DSQLITE_OS_OTHER \
    -DSQLITE_ENABLE_MEMSYS3 \
    -I$DIST_DIR/sqlite \
    -o $SCRIPT_DIR/out/bm_speedtest1.wasm $SCRIPT_DIR/app/speedtest1.c $DIST_DIR/sqlite/*.c

compileaot $SCRIPT_DIR/out/bm_speedtest1
