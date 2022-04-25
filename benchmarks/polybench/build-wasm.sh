#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "polybench (WASM)"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

docker build $DIST_DIR/webassembly-polybench-c/utilities/compile-wasm -t polybenchc-wasm-compiler

docker run \
    --rm \
    -v $DIST_DIR/webassembly-polybench-c:/polybenchc \
    -v $SCRIPT_DIR/out:/polybenchc/out \
    -v `realpath $WATZ_RUNTIME_DIR`:/wamr \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    polybenchc-wasm-compiler \
    --dataset-size MEDIUM_DATASET \
    --output /polybenchc/out \
    --call-benchmark-in-main \
    --display-time \
    --wamr /wamr \
    --aot \
    --aot-target aarch64 \
    --aot-bounds-checks 0 \
    --aot-size-level 0
