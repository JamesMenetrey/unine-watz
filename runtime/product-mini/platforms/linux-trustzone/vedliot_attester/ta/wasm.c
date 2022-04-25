#include <stdio.h>
#include "logging.h"
#include "wasm.h"
#include "tee_benchmarks.h"

wamr_context *singleton_wamr_context;

#ifdef DEBUG_MESSAGE
#define UINT8_DIGIT_MAX_SIZE 	2
static void utils_print_byte_array(uint8_t *byte_array, int byte_array_len)
{
	// +byte_array_len for spaces
	int buffer_len = UINT8_DIGIT_MAX_SIZE * byte_array_len + byte_array_len;
	char *buffer = TEE_Malloc(buffer_len, TEE_USER_MEM_HINT_NO_FILL_ZERO);

	int i, buffer_cursor = 0;
	for (i = 0; i < byte_array_len; ++i)
	{
		buffer_cursor += snprintf(buffer + buffer_cursor, buffer_len - buffer_cursor, "%02x ", byte_array[i]);
	}

	// Replace trhe last space by the string termination char
	buffer[buffer_cursor] = '\0';

	DMSG("[%s]", buffer);

	TEE_Free(buffer);
}
#endif

void TA_SetOutputBuffer(void *output_buffer, uint64_t output_buffer_size) {
    vedliot_set_output_buffer(output_buffer, output_buffer_size);
}

TEE_Result TA_HashWasmBytecode(wamr_context *ctx) {
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle operation_handle;
    uint32_t expected_digest_len = RA_HASH_SIZE / 8;
	uint32_t digest_len = RA_HASH_SIZE;

    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    res = TEE_DigestDoFinal(operation_handle, ctx->wasm_bytecode, ctx->wasm_bytecode_size, ctx->wasm_bytecode_hash, &digest_len);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_DigestDoFinal failed. Error: %x", res);
        goto out;
    }

    if (digest_len != expected_digest_len) {
        EMSG("The hash size does not correspond to the expected value (actual: %d; expected: %d).", digest_len, expected_digest_len);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("Dumping WASM bytecode hash:");
    utils_print_byte_array(ctx->wasm_bytecode_hash, digest_len);
#endif

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

TEE_Result TA_InitializeWamrRuntime(wamr_context* context, int argc, char** argv)
{
    RuntimeInitArgs init_args;
    TEE_MemFill(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = context->heap_buf;
    init_args.mem_alloc_option.pool.heap_size = context->heap_size;

    // Native symbols need below registration phase
    init_args.n_native_symbols = context->native_symbols_size / sizeof(NativeSymbol);
    init_args.native_module_name = "env";
    init_args.native_symbols = context->native_symbols;

    if (!wasm_runtime_full_init(&init_args)) {
        EMSG("Init runtime environment failed.\n");
        return TEE_ERROR_GENERIC;
    }


#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_END_INIT));
#endif

    char error_buf[128];
    context->module = wasm_runtime_load(context->wasm_bytecode, context->wasm_bytecode_size, error_buf, sizeof(error_buf));
    if(!context->module) {
        EMSG("Load wasm module failed. error: %s\n", error_buf);
        return TEE_ERROR_GENERIC;
    }

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_END_LOAD));
#endif

    wasm_runtime_set_wasi_args(context->module, NULL, 0, NULL, 0, NULL, 0, argv, argc);

    uint32_t stack_size = 256 * 1024, heap_size = 8096;
    context->module_inst = wasm_runtime_instantiate(context->module,
                                         stack_size,
                                         heap_size,
                                         error_buf,
                                         sizeof(error_buf));
    if (!context->module_inst) {
        EMSG("Instantiate wasm module failed. error: %s\n", error_buf);
        return TEE_ERROR_GENERIC;
    }

#ifdef PROFILING_LAUNCH_TIME
    TEE_GetREETime(benchmark_get_store(PROFILING_LAUNCH_TIME_END_INSTANTIATE));
#endif

    singleton_wamr_context = context;

    return TEE_SUCCESS;
}

TEE_Result TA_ExecuteWamrRuntime(wamr_context* context)
{
    if (!wasm_application_execute_main(context->module_inst, 0, NULL))
    {
        EMSG("call wasm entry point test failed. %s\n", wasm_runtime_get_exception(context->module_inst));
        return TEE_ERROR_GENERIC;
    }

    return TEE_SUCCESS;
}

void TA_TearDownWamrRuntime(wamr_context* context)
{
    singleton_wamr_context = NULL;

    if (context->module_inst)
    {
        wasm_runtime_deinstantiate(context->module_inst);
    }

    if (context->module) wasm_runtime_unload(context->module);
    wasm_runtime_destroy();
}
