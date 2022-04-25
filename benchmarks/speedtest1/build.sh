#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

./build-runtime.sh
./build-native-ree.sh
./build-native-tee.sh
./build-wasm.sh
