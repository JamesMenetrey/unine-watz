#!/bin/bash
SCRIPT_PATH=$(dirname $(realpath $0))
ROOT_DIR=$SCRIPT_PATH/..
OUTPUT_PATH=$ROOT_DIR/output

# exit when any command fails
set -e

MAKEFILES=$(find $ROOT_DIR -name Makefile)

mkdir -p $OUTPUT_PATH

for makefile in $MAKEFILES
do
    MAKEFILE_FOLDER=$(dirname $makefile)
    BENCHMARK_BINARY=$(basename $MAKEFILE_FOLDER)
    cd $MAKEFILE_FOLDER
    make clean
    make
    mv $BENCHMARK_BINARY $OUTPUT_PATH
    cd $ROOT_DIR
done