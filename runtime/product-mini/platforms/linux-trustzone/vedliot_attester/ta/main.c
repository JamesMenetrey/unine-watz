#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "wasm_export.h"
#include "bh_platform.h"

#include <wamr_ta.h>
#include <wasm.h>

#include "logging.h"
#include "ra_wasi.h"
#include "remote_attestation.h"
#include "tee_benchmarks.h"

static uint32_t heap_size;

TEE_Result TA_CreateEntryPoint(void) {
    DMSG("has been called");
    return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void) {
    DMSG("has been called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx) {
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE);
    DMSG("has been called");

    if (param_types != exp_param_types)
      return TEE_ERROR_BAD_PARAMETERS;

    (void)&params;
    (void)&sess_ctx;

    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx) {
    (void)&sess_ctx;
    DMSG("Goodbye!");
}

static TEE_Result TA_SetHeapSize(uint32_t size) {
    heap_size = size;
    DMSG("The heap set is set to %u", heap_size);

    return TEE_SUCCESS;
}

static TEE_Result TA_RunWasm(uint8_t* wasm_bytecode, uint32_t wasm_bytecode_size, char* arg_buff, void *output_buffer,
        uint64_t output_buffer_size, void *benchmark_buffer, uint64_t benchmark_buffer_size)
{
    (void)&benchmark_buffer;
    (void)&benchmark_buffer_size;
    DMSG("has been called");

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_START_MEMORY));
#endif
    
    // Allocate secure memory locations
    uint8_t *global_heap_buf = TEE_Malloc(heap_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    uint8_t *trusted_wasm_bytecode = TEE_Malloc(wasm_bytecode_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    
    // Copy the shared memory that contains the WASM bytecode into the secure memory
    TEE_MemMove(trusted_wasm_bytecode, wasm_bytecode, wasm_bytecode_size);

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_END_MEMORY));
#endif

    // Set the output buffer to gather the stdout once the application ended
    TA_SetOutputBuffer(output_buffer, output_buffer_size);

    // General settings for the runtime
    TEE_Result result;
    wamr_context context =
    {
        .heap_buf = global_heap_buf,
        .heap_size = heap_size,
        .native_symbols = wasi_ra_native_symbols,
        .native_symbols_size = wasi_ra_native_symbols_size,
        .wasm_bytecode = trusted_wasm_bytecode,
        .wasm_bytecode_size = wasm_bytecode_size
    };

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_START_HASH));
#endif

    // Hash the WASM bytecode for future RA quotes
    result = TA_HashWasmBytecode(&context);
    if (result != TEE_SUCCESS) goto error;

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_END_HASH));
#endif

    DMSG("TA_InitializeWamrRuntime\n");
    int argc = arg_buff != NULL ? 2 : 1;
    char* argv[] = {(char*)"", arg_buff};
    result = TA_InitializeWamrRuntime(&context, argc, argv);
    if (result != TEE_SUCCESS) goto error;

    DMSG("TA_ExecuteWamrRuntime\n");
    result = TA_ExecuteWamrRuntime(&context);
    if (result != TEE_SUCCESS) goto error;

#ifdef PROFILING_LAUNCH_TIME
    snprintf(benchmark_buffer, benchmark_buffer_size, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,", \
        benchmark_get_value(PROFILING_LAUNCH_TIME_START_MEMORY), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_END_MEMORY), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_START_HASH), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_END_HASH), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_END_INIT), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_END_LOAD), \
        benchmark_get_value(PROFILING_LAUNCH_TIME_END_INSTANTIATE));
#endif

#ifdef PROFILING_MESSAGES
    snprintf(benchmark_buffer, benchmark_buffer_size, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", \
            benchmark_get_value(PROFILING_MESSAGES_QUOTE_START), \
            benchmark_get_value(PROFILING_MESSAGES_QUOTE_END), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_MEM_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_KEYGEN_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_KEYGEN_END), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_MEM_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_ASYM_CRYPTO_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_KEYGEN_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_END), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_MEM_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_START), \
            benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_END));
#endif

#ifdef PROFILING_MESSAGE3
    snprintf(benchmark_buffer, benchmark_buffer_size, "%ld,%ld,%ld,%ld,", \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC_START), \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC_END), \
            benchmark_get_value(PROFILING_MESSAGE3_DECRYPT_START), \
            benchmark_get_value(PROFILING_MESSAGE3_DECRYPT_END));
#endif

    error:
    DMSG("TA_TearDownWamrRuntime\n");
    TA_TearDownWamrRuntime(&context);

    // Free up the allocated resources
    TEE_Free(global_heap_buf);
    TEE_Free(trusted_wasm_bytecode);

    return result;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types,
     TEE_Param params[4])
{
    (void)&sess_ctx;
    uint32_t exp_param_types = 0;

    switch (cmd_id) {
    case COMMAND_RUN_WASM:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT,
					     TEE_PARAM_TYPE_MEMREF_INOUT, TEE_PARAM_TYPE_MEMREF_INOUT);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;
        
        return TA_RunWasm((unsigned char*)params[0].memref.buffer,
            params[0].memref.size,
            (char*)params[1].memref.buffer,
            params[2].memref.buffer,
            params[2].memref.size,
            params[3].memref.buffer,
            params[3].memref.size);
    
    case COMMAND_CONFIGURE_HEAP:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT, TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        return TA_SetHeapSize(params[0].value.a);
    default:
        return TEE_ERROR_BAD_PARAMETERS;
  }
}
