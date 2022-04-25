#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

buildaotcompiler
buildwatz $((12 * 1024 * 1024)) $((2 * 1024 * 1024))
buildwamr
