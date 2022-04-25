#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "messages012-time"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

/opt/wasi-sdk/bin/clang $BM_CFLAGS \
    --target=wasm32-wasi \
    --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
    -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
    -Wl,--strip-all \
    -I $SCRIPT_DIR/app \
    -o bm_messages012.wasm $SCRIPT_DIR/app/main.c

compileaot $SCRIPT_DIR/out/bm_messages012
