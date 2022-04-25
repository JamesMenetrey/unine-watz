#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "logging.h"
#include "wasm_export.h"
#include "bh_platform.h"

#include "vedliot_verifier_ta.h"
#include "verifier.h"
#include "tee_benchmarks.h"

static ra_context_verifier context_singleton;

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
    TEE_Result res;

    res = import_attester_attestation_key(&context_singleton);
    if (res != TEE_SUCCESS) {
        EMSG("The import of the public attestation key failed. Error: %x", res);
        return res;
    }

    res = generate_ecdsa_keypair(&context_singleton);
    if (res != TEE_SUCCESS) {
        EMSG("The generation of the ECDSA keypair for the service provider failed. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx) {
    (void)&sess_ctx;

    dispose_ra_context(&context_singleton);
    DMSG("Goodbye!");
}

static TEE_Result TA_ConfigureCustomSecret(ra_context_verifier *ctx, uint8_t *secret, uint32_t secret_size) {
    ctx->secret_size = secret_size;
    ctx->secret = TEE_Malloc(secret_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    TEE_MemMove(ctx->secret, secret, secret_size);

    return TEE_SUCCESS;
}

TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4])
{
    (void)&sess_ctx;
    (void)&cmd_id;
    uint32_t exp_param_types = 0;
    TEE_Result res;

    switch (cmd_id) {
    case VERIFIER_COMMAND_BOOTSTRAP_SECRET:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        return TA_ConfigureCustomSecret(&context_singleton, params[0].memref.buffer, params[0].memref.size);

    case VERIFIER_COMMAND_HANDLE_MSG_0:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        return handle_msg0(&context_singleton, params[0].memref.buffer, params[0].memref.size);

    case VERIFIER_COMMAND_PREPARE_MSG_1:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
					     TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        uint32_t msg1_size;
        res = prepare_msg1(&context_singleton, params[0].memref.buffer, params[0].memref.size, &msg1_size);
        params[1].value.a = msg1_size;

        return res;
    
    case VERIFIER_COMMAND_HANDLE_MSG_2:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_OUTPUT,
					     TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        res = handle_msg2(&context_singleton, params[0].memref.buffer, params[0].memref.size);
        params[1].value.a = res == TEE_SUCCESS && context_singleton.is_quote_valid;

#ifdef PROFILING_MESSAGES
        snprintf(params[2].memref.buffer, params[2].memref.size, "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n", \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_KEYGEN1_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_MEM_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_KEYGEN2_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE0_KEYGEN2_END), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_MEM1_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_ASYM_CRYPTO_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_MEM2_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE1_MEM2_END), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_MEM1_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_MEM2_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_START), \
                benchmark_get_value(PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_END));
#endif

        return res;

    case VERIFIER_COMMAND_PREPARE_MSG_3:
        exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT,
					     TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);
        if (exp_param_types != param_types) return TEE_ERROR_BAD_PARAMETERS;

        return prepare_msg3(&context_singleton, params[0].memref.buffer, params[0].memref.size,
                context_singleton.secret, context_singleton.secret_size, params[1].memref.buffer, params[1].memref.size);

    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }
}
