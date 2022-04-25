#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

buildaotcompiler
buildwatz $((25 * 1024 * 1024)) $((1 * 1024 * 1024))
buildwamr
