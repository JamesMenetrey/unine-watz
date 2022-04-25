#!/bin/bash
# This script replaces the usage of the repo manifest "imx.xml".

clone () {
    url=$1
    directory=$2
    rev=$3

    git clone --recurse-submodules $url $directory
    (cd $directory && git checkout -q $3)
}

clone_shallow () {
    url=$1
    directory=$2
    rev=$3

    git -c http.sslVerify=false -c advice.detachedHead=false clone --recurse-submodules -b $rev --depth 1 $url $directory
}

clone https://github.com/OP-TEE/optee_client.git optee_client fa1d30c95d6f84cffed59220c0443709c303866c
clone https://github.com/OP-TEE/optee_test.git optee_test 81e71a80cb070e71e79ffcf325e2f94deaebeeb7
clone https://github.com/linaro-swg/optee_examples.git optee_examples 5ceb1ffedf0fe5263697794a64ab47a7d35fe412
clone_shallow https://github.com/linaro-swg/linux.git linux optee
clone_shallow https://github.com/buildroot/buildroot.git buildroot 2021.02
clone_shallow https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git trusted-firmware-a v2.3
clone_shallow https://github.com/u-boot/u-boot.git u-boot v2020.10-rc2
clone_shallow https://source.codeaurora.org/external/imx/imx-mkimage.git imx-mkimage rel_imx_5.4.24_2.1.0
