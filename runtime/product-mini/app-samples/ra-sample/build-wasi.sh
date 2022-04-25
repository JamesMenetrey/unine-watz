# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

WAMR_DIR=${PWD}/../../..

echo "Build wasm app .."
/opt/wasi-sdk/bin/clang -O3 \
        --target=wasm32-wasi \
        --sysroot=/opt/wasi-sdk/share/wasi-sysroot/ \
        -Wl,--allow-undefined-file=/opt/wasi-sdk/share/wasi-sysroot/share/wasm32-wasi/defined-symbols.txt \
        -Wl,--strip-all \
        -o ra-sample.wasm main.c

echo "Build binarydump tool .."
rm -fr build && mkdir build && cd build
cmake ../../../../test-tools/binarydump-tool
make
cd ..

echo "Generate test_wasm.h .."
./build/binarydump -o ra-sample.h -n wasm_test_file ra-sample.wasm

echo "Done"
