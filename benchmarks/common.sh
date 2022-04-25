#!/bin/bash

# USER SETTINGS
BM_BOARD_HOSTNAME="<IP or HOSTNAME>"
BM_BUILDER_HOSTNAME="<IP or HOSTNAME>"
BM_BOARD_USER="root"
BM_BUILDER_PATH="/opt/watz/benchmarks"

# exit when any command fails
set -e

# Common settings
TA_LOG_LEVEL=2

# define common paths
BM_CFLAGS="-O3"
ROOT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
OPTEE_DIR=/opt/watz
OPTEE_OS_DIR=$OPTEE_DIR/optee_os
OPTEE_TOOLCHAINS_DIR=$OPTEE_DIR/toolchains
OPTEE_BR_OUT_DIR=$OPTEE_DIR/out-br
WATZ_RUNTIME_DIR=$ROOT_DIR/../runtime
DIST_DIR=$ROOT_DIR/dist
LOGS_DIR=$ROOT_DIR/logs

# Macros
announce () {
    echo "$(tput smso 2>/dev/null)>>> $1$(tput rmso 2>/dev/null)"
}

announcebuild () {
    announce "Building WaTZ benchmark: $1"
}

announcedeploy () {
    announce "Deploying WaTZ benchmark: $1"
}

announcerun () {
    announce "Running WaTZ benchmark: $1"
}

safesleep () {
    sleep 2
}

# Define the functions to build and deploy WaTZ (for TEE)
# Prototype: buildwatz <attester_data_size> <verifier_data_size> [make param1]
buildwatz () {
    announcebuild "WaTZ runtime"
    mkdir -p $WATZ_RUNTIME_DIR/product-mini/platforms/linux-trustzone/build
    cd $WATZ_RUNTIME_DIR/product-mini/platforms/linux-trustzone/build
    cmake ..
    make

    announcebuild "WaTZ attester"
    cd $WATZ_RUNTIME_DIR/product-mini/platforms/linux-trustzone/vedliot_attester
    make clean
    make CROSS_COMPILE=$OPTEE_TOOLCHAINS_DIR/aarch64/bin/aarch64-linux-gnu- \
        TEEC_EXPORT=$OPTEE_BR_OUT_DIR/host/aarch64-buildroot-linux-gnu/sysroot/usr \
        TA_DEV_KIT_DIR=$OPTEE_OS_DIR/out/arm/export-ta_arm64 TA_DATA_SIZE=$1 CFG_TEE_TA_LOG_LEVEL=$TA_LOG_LEVEL $3

    announcebuild "WaTZ verifier"
    cd $WATZ_RUNTIME_DIR/product-mini/platforms/linux-trustzone/vedliot_verifier
    make clean
    make CROSS_COMPILE=$OPTEE_TOOLCHAINS_DIR/aarch64/bin/aarch64-linux-gnu- \
        TEEC_EXPORT=$OPTEE_BR_OUT_DIR/host/aarch64-buildroot-linux-gnu/sysroot/usr \
        TA_DEV_KIT_DIR=$OPTEE_OS_DIR/out/arm/export-ta_arm64 TA_DATA_SIZE=$2 CFG_TEE_TA_LOG_LEVEL=$TA_LOG_LEVEL $3
}

deploywatz () {
    announcedeploy "WaTZ attester"
    mkdir -p $ROOT_DIR/build
    cd $ROOT_DIR/build
    rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/../runtime/product-mini/platforms/linux-trustzone/vedliot_attester/out/\* out/
    sshpass -p "$BM_BOARD_USER" rsync --progress out/ca/vedliot_attester $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/usr/bin
    sshpass -p "$BM_BOARD_USER" rsync --progress out/ta/bc20728a-6a28-49d8-98d8-f22e7535f137.ta $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/lib/optee_armtz
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME 'chmod 666 /lib/optee_armtz/bc20728a-6a28-49d8-98d8-f22e7535f137.ta'

    announcedeploy "WaTZ verifier"
    cd $ROOT_DIR/build
    rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/../runtime/product-mini/platforms/linux-trustzone/vedliot_verifier/out/\* out/
    sshpass -p "$BM_BOARD_USER" rsync --progress out/ca/vedliot_verifier $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/usr/bin
    sshpass -p "$BM_BOARD_USER" rsync --progress out/ta/526461e2-d34a-4d96-8ca3-7fb9f4b898ef.ta $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/lib/optee_armtz
    sshpass -p "$BM_BOARD_USER" ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME 'chmod 666 /lib/optee_armtz/526461e2-d34a-4d96-8ca3-7fb9f4b898ef.ta'
}

# Define the functions to build and deploy WAMR (for REE)
buildwamr () {
    announcebuild "WAMR runtime"
    mkdir -p $WATZ_RUNTIME_DIR/product-mini/platforms/linux/build
    cd $WATZ_RUNTIME_DIR/product-mini/platforms/linux/build
    cp ../CMakeLists-aarch64.txt ../CMakeLists.txt
    cmake ..
    make clean
    make
    rm ../CMakeLists.txt
}

deploywamr () {
    announcedeploy "WAMR runtime"
    mkdir -p $WATZ_RUNTIME_DIR/product-mini/platforms/linux/build
    cd $WATZ_RUNTIME_DIR/product-mini/platforms/linux/build
    rsync --progress -r $BM_BUILDER_HOSTNAME:$BM_BUILDER_PATH/../runtime/product-mini/platforms/linux/build/iwasm .
    sshpass -p "$BM_BOARD_USER" rsync --progress iwasm $BM_BOARD_USER@$BM_BOARD_HOSTNAME:/usr/bin
}

buildaotcompiler () {
    cd $WATZ_RUNTIME_DIR/wamr-compiler
    
    if [ ! -d "../core/deps/llvm" ]; then
        announce "Building LLVM"
        ./build_llvm.sh
    fi

    announce "Building AoT compiler"
    mkdir -p build
    cd build
    cmake ..
    make
}

compileaot () {
    # Configure the bounds checks similarly to SGX.
    # Use the large size; small and tiny cannot be properly compiled.
    $WATZ_RUNTIME_DIR/wamr-compiler/build/wamrc \
        --target=aarch64 \
        --bounds-checks=0 \
        --size-level=0 \
        -o $1.aot \
        $1.wasm
}

restart_tee_supplicant() {
    sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "killall tee-supplicant"
    safesleep
    sshpass -p $BM_BOARD_USER ssh $BM_BOARD_USER@$BM_BOARD_HOSTNAME "/etc/init.d/S30optee start"
    safesleep
}

# Set up the common environment
mkdir -p $LOGS_DIR
