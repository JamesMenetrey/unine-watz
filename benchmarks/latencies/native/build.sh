#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../../common.sh

announcebuild "latencies-native"

cd $SCRIPT_DIR
make CROSS_COMPILE=$OPTEE_TOOLCHAINS_DIR/aarch64/bin/aarch64-linux-gnu- \
    TEEC_EXPORT=$OPTEE_BR_OUT_DIR/host/aarch64-buildroot-linux-gnu/sysroot/usr \
    TA_DEV_KIT_DIR=$OPTEE_OS_DIR/out/arm/export-ta_arm64
