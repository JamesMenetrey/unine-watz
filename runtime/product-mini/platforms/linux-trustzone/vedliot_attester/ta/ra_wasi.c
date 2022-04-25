#include "logging.h"
#include "ra_wasi.h"
#include "wasm.h"

static bool is_ra_context_available = true;
static bool is_ra_quote_available = true;
static ra_context ra_context_singleton;
static ra_quote ra_quote_singleton;
static TEE_TASessionHandle attestation_session;

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

	// Replace the last space by the string termination char
	buffer[buffer_cursor] = '\0';

	DMSG("[%s]", buffer);

	TEE_Free(buffer);
}
#endif

WASI_RA_Result wasi_ra_collect_quote(wasm_exec_env_t exec_env, uint8_t *anchor, int anchor_size, uint32_t quote_handle_out) {
    DMSG("has been called");

    TEE_Result res;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!wasm_runtime_validate_app_addr(module_inst, quote_handle_out, sizeof(uint32_t))) return WASI_RA_SANDBOX_VIOLATION;
    int *quote_handle = wasm_runtime_addr_app_to_native(module_inst, quote_handle_out);

    if (!is_ra_quote_available) return WASI_RA_CONCURRENT_ERROR;
    is_ra_quote_available = false;

    res = ra_quote_collect(&attestation_session, singleton_wamr_context->wasm_bytecode_hash, RA_HASH_SIZE / 8,
            anchor, anchor_size, &ra_quote_singleton);
    if (res != TEE_SUCCESS) {
        EMSG("The attestation service cannot compute the quote. Error: %x", res);
        return WASI_RA_GENERIC_ERROR;
    }

#ifdef DEBUG_MESSAGE
    DMSG("wasi_ra_collect_quote called! anchor address: %p; size: %d; value: %s. Dumping quote:", anchor, anchor_size, anchor);
    utils_print_byte_array((uint8_t*) &ra_quote_singleton, sizeof(ra_quote) / 2);
    utils_print_byte_array(((uint8_t*) &ra_quote_singleton) + sizeof(ra_quote) / 2, sizeof(ra_quote) / 2);
#endif

    *quote_handle = WASI_RA_DEFAULT_QUOTE_HANDLE;

    return WASI_RA_SUCCESS;
}

WASI_RA_Result wasi_ra_net_dispose_quote(wasm_exec_env_t exec_env, uint32_t quote_handle) {
    DMSG("has been called");

    (void)exec_env;
    TEE_Result res;
    ra_quote *quote;

    if (is_ra_quote_available) return WASI_RA_INVALID_QUOTE_HANDLE;

    if (quote_handle == WASI_RA_DEFAULT_QUOTE_HANDLE) {
        quote = &ra_quote_singleton;
    } else {
        EMSG("The ra_quote handle '%u' is invalid.", quote_handle);
        return WASI_RA_INVALID_QUOTE_HANDLE;
    }

    res = ra_quote_dispose(&attestation_session, quote);
    if (res != TEE_SUCCESS) {
        EMSG("The quote cannot be disposed. Error: %x", res);
        return WASI_RA_GENERIC_ERROR;
    }

    is_ra_quote_available = true;

    return WASI_RA_SUCCESS;
}

WASI_RA_Result wasi_ra_net_handshake(wasm_exec_env_t exec_env, const char* host, uint8_t *ecdsa_service_public_key_x,
        uint32_t ecdsa_service_public_key_x_size, uint8_t *ecdsa_service_public_key_y, uint32_t ecdsa_service_public_key_y_size,
        uint8_t* anchor_buffer_out, uint32_t anchor_buffer_size, uint32_t ra_context_handle_out) {
    DMSG("has been called");

    TEE_Result res;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!wasm_runtime_validate_app_addr(module_inst, ra_context_handle_out, sizeof(uint32_t))) return WASI_RA_SANDBOX_VIOLATION;
    int *ta_context_handle = wasm_runtime_addr_app_to_native(module_inst, ra_context_handle_out);

    if (!is_ra_context_available) return WASI_RA_CONCURRENT_ERROR;

    res = ra_net_handshake(&ra_context_singleton, host, ecdsa_service_public_key_x, ecdsa_service_public_key_x_size,
            ecdsa_service_public_key_y, ecdsa_service_public_key_y_size, anchor_buffer_out, anchor_buffer_size);
    if (res != TEE_SUCCESS) {
        EMSG("The handshake for the remote attestation throws an error: %x", res);
        return WASI_RA_GENERIC_ERROR;
    }

    *ta_context_handle = WASI_RA_DEFAULT_RA_CONTEXT_HANDLE;
    is_ra_context_available = false;

    return WASI_RA_SUCCESS;
}

WASI_RA_Result wasi_ra_net_send_quote(wasm_exec_env_t exec_env, uint32_t ra_context_handle, uint32_t quote_handle) {
    DMSG("has been called");

    (void)exec_env;
    TEE_Result res;
    ra_context *context;
    ra_quote *quote;

    if (is_ra_context_available) return WASI_RA_INVALID_RA_CONTEXT_HANDLE;

    if (ra_context_handle == WASI_RA_DEFAULT_RA_CONTEXT_HANDLE) {
        context = &ra_context_singleton;
    } else {
        EMSG("The ra_context handle '%u' is invalid.", ra_context_handle);
        return WASI_RA_INVALID_RA_CONTEXT_HANDLE;
    }

    if (quote_handle == WASI_RA_DEFAULT_QUOTE_HANDLE) {
        quote = &ra_quote_singleton;
    } else {
        EMSG("The ra_quote handle '%u' is invalid.", quote_handle);
        return WASI_RA_INVALID_QUOTE_HANDLE;
    }

    res = ra_net_send_quote(context, quote);
    if (res != TEE_SUCCESS) {
        EMSG("The sending of the remote attestation quote has failed. Error: %x", res);
        return WASI_RA_GENERIC_ERROR;
    }

    return WASI_RA_SUCCESS;

}

WASI_RA_Result wasi_ra_net_receive_data(wasm_exec_env_t exec_env, uint32_t ra_context_handle, uint32_t data_out, uint32_t data_size_inout) {
    DMSG("has been called");

    TEE_Result res;
    ra_context *context;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);

    if (!wasm_runtime_validate_app_addr(module_inst, data_size_inout, sizeof(uint32_t))) return WASI_RA_SANDBOX_VIOLATION;
    uint32_t *data_size = wasm_runtime_addr_app_to_native(module_inst, data_size_inout);

    if (!wasm_runtime_validate_app_addr(module_inst, data_out, *data_size)) return WASI_RA_SANDBOX_VIOLATION;
    uint8_t *data = wasm_runtime_addr_app_to_native(module_inst, data_out);

    if (is_ra_context_available) return WASI_RA_INVALID_RA_CONTEXT_HANDLE;

    if (ra_context_handle == WASI_RA_DEFAULT_RA_CONTEXT_HANDLE) {
        context = &ra_context_singleton;
    } else {
        EMSG("The ra_context handle '%u' is invalid.", ra_context_handle);
        return WASI_RA_INVALID_RA_CONTEXT_HANDLE;
    }

    res = ra_net_receive_data(context, data, data_size);
    if (res != TEE_SUCCESS) {
        EMSG("The receival of the data after a remote attestation has failed. Error: %x", res);
        return WASI_RA_GENERIC_ERROR;
    }

    return WASI_RA_SUCCESS;
}

WASI_RA_Result wasi_ra_net_dispose(wasm_exec_env_t exec_env, uint32_t ra_context_handle) {
    DMSG("has been called");

    (void)exec_env;
    TEE_Result res;
    ra_context *context;

    if (is_ra_context_available) return WASI_RA_INVALID_RA_CONTEXT_HANDLE;

    if (ra_context_handle == WASI_RA_DEFAULT_RA_CONTEXT_HANDLE) {
        context = &ra_context_singleton;
    } else {
        EMSG("The ra_context handle '%u' is invalid.", ra_context_handle);
        return WASI_RA_INVALID_RA_CONTEXT_HANDLE;
    }

    res = ra_net_dispose(context);
    if (res != TEE_SUCCESS) {
        EMSG("The remote attestation context with handle '%u' cannot be disposed. Error: %x", ra_context_handle, res);
        return WASI_RA_GENERIC_ERROR;
    }

    is_ra_context_available = true;

    return WASI_RA_SUCCESS;
}

NativeSymbol wasi_ra_native_symbols[] =
{
    EXPORT_WASM_API_WITH_SIG(wasi_ra_collect_quote, "(*~i)i"),
    EXPORT_WASM_API_WITH_SIG(wasi_ra_net_dispose_quote, "(i)i"),
    EXPORT_WASM_API_WITH_SIG(wasi_ra_net_handshake, "($*~*~*~i)i"),
    EXPORT_WASM_API_WITH_SIG(wasi_ra_net_send_quote, "(ii)i"),
    EXPORT_WASM_API_WITH_SIG(wasi_ra_net_receive_data, "(iii)i"),
    EXPORT_WASM_API_WITH_SIG(wasi_ra_net_dispose, "(i)i")
};

const size_t wasi_ra_native_symbols_size = sizeof(wasi_ra_native_symbols);