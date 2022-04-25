#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "message3-time"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

# Prototype: build <size_in_KB>
build () {
    kilobytes=$1
    bytes=$(($kilobytes * 1024))

    echo "#define DATA_SIZE $bytes" > $SCRIPT_DIR/app/data_size.h

    /opt/wasi-sdk/bin/clang $BM_CFLAGS \
        --target=wasm32-wasi \
        --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
        -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
        -Wl,--strip-all \
        -I $SCRIPT_DIR/app \
        -o bm_message3_$kilobytes.wasm $SCRIPT_DIR/app/main.c

    compileaot $SCRIPT_DIR/out/bm_message3_$kilobytes
}

for KB in {512..3072..512}
do
    build $KB
done
