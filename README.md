# WaTZ: A Trusted WebAssembly Runtime Environment with Remote Attestation for TrustZone

DOI: [10.1109/ICDCS54860.2022.00116](https://doi.ieeecomputersociety.org/10.1109/ICDCS54860.2022.00116)

[IEEE Computer Society](https://www.computer.org/csdl/proceedings-article/icdcs/2022/717700b177/1HriZ4jSthK) | [arXiv](https://arxiv.org/abs/2206.08722)

The paper version will be published in the proceedings of the 42nd IEEE International Conference on Distributed Computing Systems (ICDCS'22).


## Reproducibility of the results
The results shown in the paper can be reproduced using the [benchmarks' instructions](benchmarks/).

The fork of the [runtime](runtime/) is based on the revision [cba4c782](https://github.com/bytecodealliance/wasm-micro-runtime/tree/cba4c782).  
The fork of [optee_os](optee_os/) is based on the revision [3af354e3](https://github.com/OP-TEE/optee_os/tree/3af354e3).  
The fork of [build](build/) is based on the revision [af24ff9](https://github.com/OP-TEE/build/tree/af24ff9).  

## License

The source code of TWINE and the source code of the benchmarks are released under the Apache license 2.0.
Check the file `LICENSE` for more information.

## Authors

{Jämes Ménétrey, Marcelo Pasin, Pascal Felber, Valerio Schiavoni} @ University of Neuchâtel, Switzerland

## Funding

This software artefact incorporates results from the VEDLIoT project, which received funding from the European Union's Horizon 2020 research and innovation programme under grant agreement No 957197.  
[www.vedliot.eu](https://www.vedliot.eu)