#!/bin/bash
SCRIPT_PATH=$(dirname $(realpath $0))

# exit when any command fails
set -e

cd $SCRIPT_PATH
utilities/makefile-gen.pl .
CC=clang CXX=clang++ utilities/compile-all.sh