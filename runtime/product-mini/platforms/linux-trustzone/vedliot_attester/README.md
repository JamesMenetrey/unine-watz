# Minimal Working Example (MWE) of TZ app

The MWE can be build in two ways: either by integrating it into OP-TEE's
toolchain or by doing an out-of-tree build.

## Building app out-of-tree

An out-of-tree build of the repository is very similar to the make recipes
above. The repository can easily be cross-compiled by running the commands of
the recipes. Since the repository is not integrated into OP-TEE's toolchain,
all mandatory variables have to be expanded manually. After building and
cross-compiling the repository, the executable(s) and librarie(s) have to be
installed manually into the root file system of the target platform. After
flushing the boot partition and root file system onto a SD card, the
executable(s) and librarie(s) can be copied to their appropriate places.

### Prerequisites

Ensure that the `out/` folder has been generated for the `optee_client` repository.
Change into the OP-TEE `build` repository directory and generate the output:

```
make optee-client-common
```

### Cross-compilation

Client applications (CA): `/usr/bin`
Trusted applications (TA): `/lib/optee_armtz`
Trusted shared libraries: ???

The required variables for building and cross-compiling the repository are
listed below:

`CROSS_COMPILE` (mandatory): path to the cross-compilation toolchain
- `CROSS_COMPILE=$(shell pwd)/../toolchains/aarch64/bin/aarch64-linux-gnu-`

`TEEC_EXPORT` (mandatory): path to the TEE Client API library
- TEEC_EXPORT=$(shell pwd)/../optee_client/out/export

`CFG_TEE_TA_LOG_LEVEL` (optional)

`TA_DEV_KIT_DIR` (mandatory): path to the trusted application development kit
- `TA_DEV_KIT_DIR=$(shell pwd)/../optee_os/out/arm/export-ta_arm64`

Verbose output `V` (optional):
- `V=1`

Specific compilers, otherwise fallback to default cross compiler:
- `HOST_CROSS_COMPILE` (optional)
- `LIB_CROSS_COMPILE` (optional)
- `TA_CROSS_COMPILE` (optional)


### Example of commands

```
V=1 CROSS_COMPILE=/home/.../toolchains/aarch64/bin/aarch64-linux-gnu- TEEC_EXPORT=/home/.../optee_client/out/export TA_DEV_KIT_DIR=/home/.../optee_os/out/arm/export-ta_arm64 make

make CROSS_COMPILE=/path/to/optee/toolchains/aarch64/bin/aarch64-linux-gnu- TEEC_EXPORT=/path/to/optee/optee_client/out/export/usr TA_DEV_KIT_DIR=/path/to/optee/optee_os/out/arm/export-ta_arm64
```

## Building app as part of OP-TEE

Integrating the MWE into OP-TEE's toolchain requires modifying the manifest and
build repository. It is assumed that this repositories resides with the other
OP-TEE repositories, i.e., optee_os, optee_client, optee_example, etc. First
manifest needs to have an entry for the MWE to pull the repositories together
with all other OP-TEE repositories. This modification has to be made in the
individual paltform files, e.g. manifest/qemu_v8.xml:

<remote name="<my-remote>" fetch="https://my.remote.url" />

<project path="optee_mwe" name="OP-TEE/optee_mwe.git" revision="refs/tags/<my-tag>" clone-depth="1" />

Next, MWE must be included as package into buildroot. The modifications need to
be made in the build/common.mk file:

@echo "BR2_PACKAGE_X_CROSS_COMPILE=\"$(CROSS_COMPILE_S_USER)\"" >> ../out-br/extra.conf
@echo "BR2_PACKAGE_X_SDK=\"$(OPTEE_OS_TA_DEV_KIT_DIR)\"" >> ../out-br/extra.conf
@echo "BR2_PACKAGE_X_SITE=\"$(<variable with path to X>)\"" >> ../out-br/extra.conf
@echo "BR2_PACKAGE_X=y" >> ../out-br/extra.conf

To complete the integration recipes can be added to build/common.mk to build and
clean the repository:

.PHONY: optee-mwe-common
optee-mwe-common:
	$(MAKE) -C $(<variable with path to X>) HOST_CROSS_COMPILE=... LIB_CROSS_COMPILE=... TA_CROSS_COMPILE=... TA_DEV_KIT_DIR=$(OPTEE_OS_TA_DEV_KIT_DIR) TEEC_EXPORT=$(OPTEE_CLIENT_EXPORT)

.PHONY: optee-mwe-clean-common
optee-mwe-clean-common:
	$(MAKE) -C $(<variable with path to X>) TA_DEV_KIT_DIR=$(OPTEE_OS_TA_DEV_KIT_DIR) clean
