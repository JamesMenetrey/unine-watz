#ifndef WASM_H
#define WASM_H

#include <tee_internal_api.h>

#include "wasm_export.h"

#define RA_HASH_SIZE    256

typedef struct wamr_context_
{
    uint8_t *wasm_bytecode;
    uint32_t wasm_bytecode_size;
    uint8_t wasm_bytecode_hash[RA_HASH_SIZE / 8];
    void *heap_buf;
    uint32_t heap_size;
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    NativeSymbol *native_symbols;
    uint32_t native_symbols_size;
} wamr_context;

extern wamr_context *singleton_wamr_context;

void TA_SetOutputBuffer(void *output_buffer, uint64_t output_buffer_size);
TEE_Result TA_HashWasmBytecode(wamr_context *ctx);
TEE_Result TA_InitializeWamrRuntime(wamr_context* context, int argc, char** argv);
TEE_Result TA_ExecuteWamrRuntime(wamr_context* context);
void TA_TearDownWamrRuntime(wamr_context* context);

#endif /* WASM_H */