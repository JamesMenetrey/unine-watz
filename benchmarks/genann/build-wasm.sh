#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "Genann/REE (WASM)"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

/opt/wasi-sdk/bin/clang $BM_CFLAGS \
    --target=wasm32-wasi \
    --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
    -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
    -Wl,--strip-all \
    -DCFG_TEE_TA_LOG_LEVEL=2 \
    -DREE \
    -I$DIST_DIR/webassembly-genann \
    -o $SCRIPT_DIR/out/bm_genann_ree.wasm $DIST_DIR/webassembly-genann/genann.c $SCRIPT_DIR/app/example4.c

compileaot $SCRIPT_DIR/out/bm_genann_ree

announcebuild "Genann/TEE (WASM)"
/opt/wasi-sdk/bin/clang $BM_CFLAGS \
    --target=wasm32-wasi \
    --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
    -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
    -Wl,--strip-all \
    -DCFG_TEE_TA_LOG_LEVEL=2 \
    -DTEE \
    -I$DIST_DIR/webassembly-genann \
    -o $SCRIPT_DIR/out/bm_genann_tee.wasm $DIST_DIR/webassembly-genann/genann.c $SCRIPT_DIR/app/example4.c

compileaot $SCRIPT_DIR/out/bm_genann_tee
