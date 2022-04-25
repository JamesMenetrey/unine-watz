#!/bin/bash

ROOT_DIR=$(dirname $(realpath $0))

docker build $ROOT_DIR -t polybenchc-wasm-compiler

docker run \
    --rm \
    -v $ROOT_DIR/../../:/polybenchc \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    polybenchc-wasm-compiler \
    "$@"