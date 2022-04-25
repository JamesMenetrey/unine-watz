global-incdirs-y += include
global-incdirs-y += ../../../../../core/iwasm/include/ ../../../../../core/app-framework/base/app
srcs-y += ra_wasi.c remote_attestation.c tee_benchmarks.c wasm.c main.c

# Method 2 includes the static (trusted) library between the --start-group and
# --end-group arguments.
libnames += vmlib
libdirs += ../../build/
libdeps += ../../build/libvmlib.a