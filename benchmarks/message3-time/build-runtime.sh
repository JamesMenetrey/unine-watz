#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

buildaotcompiler
buildwatz $((14 * 1024 * 1024)) $((13 * 1024 * 1024)) PROFILING=MESSAGE3
