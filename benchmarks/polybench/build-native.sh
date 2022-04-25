#!/bin/bash

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $SCRIPT_DIR/../common.sh

announcebuild "Polybench (Native)"
mkdir -p $SCRIPT_DIR/out
cd $SCRIPT_DIR/out

rm -rf $DIST_DIR/polybench-c/output
cd $DIST_DIR/polybench-c

$SCRIPT_DIR/makefile-gen.pl `realpath $DIST_DIR/polybench-c` -cfg
utilities/compile-all.sh
