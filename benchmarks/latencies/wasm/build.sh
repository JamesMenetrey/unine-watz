#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../../common.sh

buildaotcompiler
buildwatz $((2 * 1024 * 1024)) $((2 * 1024 * 1024))
buildwamr

announcebuild "latencies-wasm"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

/opt/wasi-sdk/bin/clang $BM_CFLAGS \
        --target=wasm32-wasi \
        --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
        -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
        -Wl,--strip-all \
        -o bm_latencies.wasm ../app/main.c

compileaot $SCRIPT_DIR/out/bm_latencies
