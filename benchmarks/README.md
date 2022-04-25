# Benchmarking tools for WaTZ
This folder contains the experiments illustrated in the paper **WaTZ: A Trusted WebAssembly Runtime Environment with Remote Attestation for TrustZone**.
This README describes the steps to reproduce the results of the experiments.



## Building architecture
The architecture to build this project is divided into three actors:

- the **building machine**, which compiles the software stack (i.e., WaTZ components),
- the **development board**, running WaTZ on an Arm processor with TrustZone, and
- the **deployment machine**, dedicated to download and flash WaTZ, as well as coordinates the experiments on the development board.

The benchmarking workflow is illustrated in the following diagram.

```
                      +------------------------+
                      |                        |
                      |                        |
                      |    Building machine    +---------+
                      |                        |         |
                      |                        |         |       rsync
                      +------------------------+         | Compiled artefacts
                                                         |
                                                         |
                                                         v
+------------------------+                  +------------------------+
|                        |       SSH        |                        |
|                        |   experiments    |                        |
|   Development board    |<---------------->|   Deployment machine   |
|                        |                  |                        |
|                        |                  |                        |
+--------------+----+----+                  +------------+-----------+
               | SD |                                    |
               |card|<-----------------------------------+
               +----+             Flash WaTZ
```



## Prerequisites
### Development board
 - [MCIMX8M-EVK](https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-applications-processor:MCIMX8M-EVK): Evaluation Kit for the i.MX 8M Applications Processor
 - An SD card to store the bootloader, the trusted/untrusted operating systems and the experiments (tested on a [Sandisk Extreme Pro microSD A2](https://www.digitec.ch/fr/s1/product/sandisk-extreme-pro-microsd-a2-microsdxc-128-go-u3-uhs-i-cartes-memoires-9671111))
 - An SD card reader
 - A network with DHCP to communicate between the three actors. The board is configured to be wired with an ethernet cable to the network

### Building machine
- Ubuntu 20.04 LTS (tested on 20.04.4)
- Docker (tested on 20.10.16) [[instructions](https://docs.docker.com/engine/install/ubuntu/)]

Additional dependencies must be installed with the commands below.

```
sudo apt-get install -y git 

# Dependencies of OP-TEE
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install android-tools-adb android-tools-fastboot autoconf \
        automake bc bison build-essential ccache cscope curl device-tree-compiler \
        expect flex ftp-upload gdisk iasl libattr1-dev libcap-dev \
        libfdt-dev libftdi-dev libglib2.0-dev libhidapi-dev libncurses5-dev \
        libpixman-1-dev libssl-dev libtool make \
        mtools netcat ninja-build python-crypto python3-crypto python-pyelftools \
        python3-pycryptodome python3-pyelftools python3-serial \
        rsync unzip uuid-dev xdg-utils xterm xz-utils zlib1g-dev \
        libcap-ng-dev libattr1-dev ccache cmake wget

# Set up WASI-SDK
wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-12/wasi-sdk-12.0-linux.tar.gz
sudo tar -xf wasi-sdk-12.0-linux.tar.gz -C /opt/
sudo ln -s /opt/wasi-sdk-12.0 /opt/wasi-sdk
rm wasi-sdk-12.0-linux.tar.gz

```

### Deployment machine
- Ubuntu 20.04 LTS (tested on 20.04.4)
- Optional: Jupyter Notebook (benchmarks' output parsing and data visualisation)

Additional dependencies must be installed with the commands below.

```
sudo apt-get install -y git sshpass piccom
```



## Compiling WaTZ environment
On the building machine, execute the following commands.

```
# Set up WaTZ and its dependencies
sudo mkdir -p /opt/watz
sudo chown $USER /opt/watz
git clone https://github.com/JamesMenetrey/unine-watz.git /opt/watz
cd /opt/watz
./clone.sh

# Download the toolchains
cd /opt/watz/build
make -j2 toolchains

# Building OP-TEE with WaTZ custom kernel (adjust the compilation as needed)
export OPTEE_LOG_LEVEL=2
make -j `nproc` USE_PERSISTENT_ROOTFS=1 CFG_NXP_CAAM=y CFG_CRYPTO_DRIVER=n \
        CFG_TEE_CORE_LOG_LEVEL=$OPTEE_LOG_LEVEL CFG_TEE_CORE_DEBUG=n \
        CFG_TEE_TA_LOG_LEVEL=$OPTEE_LOG_LEVEL CFG_DEBUG_INFO=n \
        CFG_MUTEX_DEBUG=n CFG_UNWIND=n
```



## Deploying WaTZ environment
On the building machine, create a folder for deployment operations and transfer the compiled image of the SD card.

```
rsync --progress <deployment_machine>:/opt/watz/out/boot.img .
```

Unmount and flash the SD card:

```
# /!\ TO BE CHANGED: this is the device of your SD card (use lsblk)
export OPTEE_SD_CARD=/dev/sdXXX
ls $OPTEE_SD_CARD?* | xargs -n1 umount

# /!\ THIS CAN BE DANGEROUS IF 'OPTEE_SD_CARD' IS NOT SET PROPERLY!
sudo dd if=boot.img of=$OPTEE_SD_CARD bs=1M conv=fsync
```

Once flashed, the SD card has two partitions: (i) the bootloader, and (ii) the rootfs.

Once started, the untrusted operating system (Linux) starts an SSH server for remote connection.
The UART port of the board can also be used to interact with the untrusted OS.
Typically, one can connect to the board using the following command, where `/dev/ttyUSB0` is the device of the UART port of the board, connected using a USB cable.
The credentials of the untrusted OS are `root` as username and password.

```
picocom -b 115200 /dev/ttyUSB0
```



## Running the experiments
On the deployment machine, clone the git repository and open the benchmarks directory.

```
git clone https://github.com/JamesMenetrey/unine-watz.git
```

Edit the file `common.sh` and set the variables `BM_BOARD_HOSTNAME` with the IP address or the hostname of the Arm board and `BM_BUILDER_HOSTNAME` the IP address or the hostname of the building server.


### Time measurements in TrustZone (Figure 3)
#### Building machine
```
cd ./benchmarks/latencies
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/latencies
./deploy.sh
./run.sh
```

The output of the experiments is stored in `./benchmarks/logs/latencies` of the deployment machine.
The Jupyter Notebook `latencies_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Startup breakdown of Wasm applications in WaTZ (Figure 4)
#### Building machine
```
cd ./benchmarks/launch-time
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/launch-time
./deploy.sh
./run.sh
```

The output of the experiments is stored in `./benchmarks/logs/launch-time` of the deployment machine.
The Jupyter Notebook `launch-time_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Wasm micro-benchmarks: PolyBench/C (Figure 5)
#### Building machine
```
cd ./benchmarks/polybench
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/polybench
./deploy.sh
./run-ree-native.sh
./run-ree-wasm.sh
./run-tee.sh
```

The output of the experiments is stored in `./benchmarks/logs/polybench` of the deployment machine.
The Jupyter Notebook `polybench_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Wasm macro-benchmarks: SQLite (Figure 6)
#### Building machine
```
cd ./benchmarks/speedtest1
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/speedtest1
./deploy.sh
./run-native-ree.sh
./run-wasm-ree.sh
./run-native-tee.sh
./run-wasm-tee.sh
```

NOTE: when the TEE experiments are torn down, this creates a stack trace on the UART port, but this does not affect the results since it occurs at the end.

The output of the experiments is stored in `./benchmarks/logs/speedtest1` of the deployment machine.
The Jupyter Notebook `speedtest1_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Remote attestation micro-benchmarks (Table 3)
#### Building machine
```
cd ./benchmarks/messages012-time
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/messages012-time
./deploy.sh
./run.sh
```

The output of the experiments is stored in `./benchmarks/logs/messages012-time` of the deployment machine.
The Jupyter Notebook `messages012-time_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Remote attestation micro-benchmarks (Figure 7)
#### Building machine
```
cd ./benchmarks/message3-time
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/message3-time
./deploy.sh
./run.sh
```

The output of the experiments is stored in `./benchmarks/logs/message3-time` of the deployment machine.
The Jupyter Notebook `message3-time_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.


### Remote attestation micro-benchmarks (Table 4 & Figure 8)
#### Building machine
```
cd ./benchmarks/genann
./build.sh
```

#### Deployment machine
```
cd ./benchmarks/genann
./deploy.sh
./run-ree.sh
./run-tee.sh
```

The output of the experiments is stored in `./benchmarks/logs/genann` of the deployment machine.
The Jupyter Notebook `genann_analysis.ipynb` can be used to analyse the raw data.
An instance of Jupyter Notebook can be launched using the script: `launch-jupyter.sh`, located in the benchmarks directory.



## License
The source code of WaTZ and the source code of the benchmarks are released under the Apache license 2.0.
Check the file `LICENSE` for more information.



## Author
{Jämes Ménétrey, Marcelo Pasin, Pascal Felber, Valerio Schiavoni} @ University of Neuchâtel, Switzerland