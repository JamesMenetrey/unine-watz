#include <pta_attestation_service.h>

#include "logging.h"
#include "remote_attestation.h"
#include "tee_benchmarks.h"

#define UINT8_DIGIT_MAX_SIZE 	2
#define MAX_NUMBER_OF_BYTES_PER_LINE 50
static void utils_print_byte_array(uint8_t *byte_array, int byte_array_len)
{
	// +MAX_NUMBER_OF_BYTES_PER_LINE for spaces
	int buffer_len = UINT8_DIGIT_MAX_SIZE * MAX_NUMBER_OF_BYTES_PER_LINE + MAX_NUMBER_OF_BYTES_PER_LINE;
	char *buffer = TEE_Malloc(buffer_len, TEE_USER_MEM_HINT_NO_FILL_ZERO);
	int bytes_on_line, buffer_cursor, i = 0;
    
    while (i < byte_array_len)
    {
        buffer_cursor = 0;

        for (bytes_on_line = 0; bytes_on_line < MAX_NUMBER_OF_BYTES_PER_LINE && i < byte_array_len; ++bytes_on_line)
        {
            buffer_cursor += snprintf(buffer + buffer_cursor, buffer_len - buffer_cursor, "%02x ", byte_array[i]);
            i++;
        }

        // Replace the last space by the string termination char
        buffer[buffer_cursor] = '\0';

        IMSG("[%s]", buffer);
    }

	TEE_Free(buffer);
}

#ifdef DEBUG_MESSAGE
static void util_dump_object_attribute(TEE_ObjectHandle object, uint32_t attributeID, uint32_t size) {
    uint8_t *buffer = TEE_Malloc(size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    uint32_t buffer_size = size;
    
    TEE_Result res = TEE_GetObjectBufferAttribute(object, attributeID, buffer, &buffer_size);
    
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return;
    }
    
    utils_print_byte_array(buffer, size);

    TEE_Free(buffer);
}
#endif

static TEE_Result receive_socket_per_chunk(TEE_iSocket *socket, TEE_iSocketHandle socketHandle, 
        uint32_t chunk_size, uint8_t *data, uint32_t data_size, uint32_t *transfered_data_out) {
    TEE_Result res = TEE_SUCCESS;
    uint8_t *data_cursor = data;
    uint32_t transfered_data = 0;
    
    // Read the stream of data per chunk
    while(chunk_size > 0 && transfered_data < data_size) {
        if (transfered_data > data_size) {
            EMSG("The data buffer is too small (requested: %u).", transfered_data);
            return TEE_ERROR_SHORT_BUFFER;
        }

        res = socket->recv(socketHandle, data_cursor, &chunk_size, TEE_TIMEOUT_INFINITE);
        if (res != TEE_SUCCESS) {
            EMSG("An error occurred while retrieving data from the socket. Error: %x", res);
            return res;
        }

        transfered_data += chunk_size;
        data_cursor += chunk_size;
    }

    *transfered_data_out = transfered_data;

    return TEE_SUCCESS;
}

static TEE_Result alloc_ra_context(ra_context *ctx) {
    TEE_MemFill(ctx, 0, sizeof(ra_context));
    
    ctx->msg0.ecdh_local_public_key_x_size = sizeof(ctx->msg0.ecdh_local_public_key_x_raw);
    ctx->msg0.ecdh_local_public_key_y_size = sizeof(ctx->msg0.ecdh_local_public_key_y_raw);

    return TEE_SUCCESS;
}

static TEE_Result close_attestation_session(TEE_TASessionHandle *attestation_session) {
    if (attestation_session != TEE_HANDLE_NULL) {
        TEE_CloseTASession(*attestation_session);
    }

    return TEE_SUCCESS;
}

static TEE_Result free_ra_context(ra_context *ctx) {
    TEE_FreeTransientObject(ctx->ecdh_local_keypair_handle);
    TEE_FreeTransientObject(ctx->ecdh_shared_key_derivation_key);
    TEE_FreeTransientObject(ctx->ecdh_shared_mac_key);
    TEE_FreeTransientObject(ctx->ecdh_shared_session_key);
    TEE_FreeTransientObject(ctx->ecdsa_verifier_public_key_handle);

    if (ctx->msg3_size > 0) {
        ctx->msg3_size = 0;
        TEE_Free(ctx->msg3);
    }
    
    ctx->socket_handle.socket->close(ctx->socket_handle.ctx);

    return TEE_SUCCESS;
}

static TEE_Result generate_ecdh_keypair(ra_context *ctx) {
    TEE_Attribute curve_attr;
    TEE_Result res;

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_KEYPAIR, WASI_RA_ECDH_KEY_SIZE, &ctx->ecdh_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    if ((res = TEE_GenerateKey(ctx->ecdh_local_keypair_handle, WASI_RA_ECDH_KEY_SIZE, &curve_attr, 1)) != TEE_SUCCESS) {
        EMSG("TEE_GenerateKey failed. Error: %x", res);
        return res;
    }

    // Extract the public key
    res = TEE_GetObjectBufferAttribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg0.ecdh_local_public_key_x_raw, &ctx->msg0.ecdh_local_public_key_x_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }
    
    res = TEE_GetObjectBufferAttribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg0.ecdh_local_public_key_y_raw, &ctx->msg0.ecdh_local_public_key_y_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the ECDH keypair
    DMSG("Dumping ECDH TEE_ATTR_ECC_PRIVATE_VALUE:");
    util_dump_object_attribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PRIVATE_VALUE, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG("Dumping ECDH TEE_ATTR_ECC_PUBLIC_VALUE_X:");
    utils_print_byte_array(ctx->msg0.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
    DMSG("Dumping ECDH TEE_ATTR_ECC_PUBLIC_VALUE_Y:");
    utils_print_byte_array(ctx->msg0.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size);
#endif

    return TEE_SUCCESS;
}

static TEE_Result establish_tcp_connection(ra_context *ctx, const char *host) {
    TEE_Result res = TEE_ERROR_GENERIC;
	TEE_tcpSocket_Setup setup = { };
	uint32_t protocolError;

	setup.ipVersion = TEE_IP_VERSION_DC;
	setup.server_port = 8080;
	setup.server_addr = (char*)host;

	ctx->socket_handle.socket = TEE_tcpSocket;
	res = ctx->socket_handle.socket->open(&ctx->socket_handle.ctx, &setup, &protocolError);
	if (res != TEE_SUCCESS) {
		EMSG("The socket cannot be opened. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

static TEE_Result send_msg0(ra_context *ctx) {
    uint32_t msg0_size = sizeof(msg0_t);
    TEE_Result res;

    res = ctx->socket_handle.socket->send(ctx->socket_handle.ctx, &ctx->msg0, &msg0_size, TEE_TIMEOUT_INFINITE);
    if (res != TEE_SUCCESS) {
        EMSG("The msg0 cannot be sent through the socket. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    DMSG("== [+: DUMP MSG0 (struct size: %ld)] ======================== ", sizeof(msg0_t));
    DMSG(" - ecdh_local_public_key_x_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg0.ecdh_local_public_key_x_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_local_public_key_x_size (size: %ld): %u", sizeof(uint32_t), ctx->msg0.ecdh_local_public_key_x_size);
    DMSG(" - ecdh_local_public_key_y_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg0.ecdh_local_public_key_y_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_local_public_key_y_size (size: %ld): %u", sizeof(uint32_t), ctx->msg0.ecdh_local_public_key_y_size);
    DMSG(" - full dump:");
    utils_print_byte_array((uint8_t*) &ctx->msg0, sizeof(msg0_t));
    DMSG("== [-: DUMP MSG0] =========================================== ");
#endif

    return TEE_SUCCESS;
}

static TEE_Result receive_msg1(ra_context *ctx) {
    TEE_Result res;
    uint8_t msg1[sizeof(msg1_t)];
    uint32_t msg1_size = sizeof(msg1_t);

    // Receive msg1
    res = receive_socket_per_chunk(ctx->socket_handle.socket, ctx->socket_handle.ctx, msg1_size, msg1, msg1_size, &msg1_size);
    if (res != TEE_SUCCESS) {
        EMSG("The msg1 cannot be retrieved. Error: %x", res);
        return res;
    }

    if (msg1_size < sizeof(msg1_t)) {
        EMSG("The size of the msg1 received is smaller than the expected msg1 (expected: %ld; actual: %u).", sizeof(msg1_t), msg1_size);
        return TEE_ERROR_GENERIC;
    }

    // Parse msg1
    TEE_MemMove(&ctx->msg1, msg1, sizeof(msg1_t));

#ifdef DEBUG_MESSAGE
    DMSG("== [+: DUMP MSG1 (struct size: %ld; received: %u)] ======================== ", sizeof(msg1_t), msg1_size);
    DMSG(" - ecdh_verifier_public_key_x_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_x_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_verifier_public_key_x_size (size: %ld): %u", sizeof(uint32_t), ctx->msg1.ecdh_verifier_public_key_x_size);
    DMSG(" - ecdh_verifier_public_key_y_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_y_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_verifier_public_key_y_size (size: %ld): %u", sizeof(uint32_t), ctx->msg1.ecdh_verifier_public_key_y_size);
    DMSG(" - ecdsa_verifier_public_key_x_raw (size: %d):", WASI_RA_ECDSA_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_x_raw, WASI_RA_ECDSA_KEY_SIZE / 8);
    DMSG(" - ecdsa_verifier_public_key_x_size (size: %ld): %u", sizeof(uint32_t), ctx->msg1.ecdsa_verifier_public_key_x_size);
    DMSG(" - ecdsa_verifier_public_key_y_raw (size: %d):", WASI_RA_ECDSA_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_y_raw, WASI_RA_ECDSA_KEY_SIZE / 8);
    DMSG(" - ecdsa_verifier_public_key_y_size (size: %ld): %u", sizeof(uint32_t), ctx->msg1.ecdsa_verifier_public_key_y_size);
    DMSG(" - ecdh_signature (size: %d):", RA_SIGNATURE_SIZE);
    utils_print_byte_array(ctx->msg1.ecdh_signature, RA_SIGNATURE_SIZE);
    DMSG(" - mac (size: %d):", WASI_RA_AES_CMAC_TAG_SIZE / 8);
    utils_print_byte_array(ctx->msg1.mac, WASI_RA_AES_CMAC_TAG_SIZE / 8);
    DMSG(" - full dump:");
    utils_print_byte_array((uint8_t*) &ctx->msg1, sizeof(msg1_t));
    DMSG("== [-: DUMP MSG1] =========================================== ");
#endif

    TEE_InitRefAttribute(&ctx->ecdh_remote_public_key_x_attr, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);

    TEE_InitRefAttribute(&ctx->ecdh_remote_public_key_y_attr, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);

#ifdef DEBUG_MESSAGE
    // Dumping the ECDH remote public key
    TEE_ObjectHandle ecdh_remote_public_key_handle;
    
    // Store the remote public key
    TEE_Attribute curve_attr;
    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_PUBLIC_KEY, WASI_RA_ECDH_KEY_SIZE, &ecdh_remote_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    TEE_Attribute attributes[] = {ctx->ecdh_remote_public_key_x_attr, ctx->ecdh_remote_public_key_y_attr, curve_attr};
    res = TEE_PopulateTransientObject(ecdh_remote_public_key_handle, attributes, 3);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        goto out;
    }

    DMSG("Dumping remote TEE_ATTR_ECC_PUBLIC_VALUE_X:");
    util_dump_object_attribute(ecdh_remote_public_key_handle, TEE_ATTR_ECC_PUBLIC_VALUE_X, ctx->msg1.ecdh_verifier_public_key_x_size);
    DMSG("Dumping remote TEE_ATTR_ECC_PUBLIC_VALUE_Y:");
    util_dump_object_attribute(ecdh_remote_public_key_handle, TEE_ATTR_ECC_PUBLIC_VALUE_Y, ctx->msg1.ecdh_verifier_public_key_y_size);

out:
    TEE_FreeTransientObject(ecdh_remote_public_key_handle);
#endif

    return res;
}

static TEE_Result verify_service_public_key(ra_context *ctx, uint8_t *ecdsa_service_public_key_x, uint32_t ecdsa_service_public_key_x_size,
        uint8_t *ecdsa_service_public_key_y, uint32_t ecdsa_service_public_key_y_size) {
    // For internal testing, if no public key is passed, skip the verification
    if (ecdsa_service_public_key_x_size == 0 && ecdsa_service_public_key_y_size == 0) return TEE_SUCCESS;
    
    if (ecdsa_service_public_key_x_size != WASI_RA_ECDSA_KEY_SIZE / 8 || ecdsa_service_public_key_y_size != WASI_RA_ECDSA_KEY_SIZE / 8) {
        EMSG("The only supported public key format is TEE_ECC_CURVE_NIST_P256");
        return TEE_ERROR_COMMUNICATION;
    }
    
    if (TEE_MemCompare(ecdsa_service_public_key_x, ctx->msg1.ecdsa_verifier_public_key_x_raw, ecdsa_service_public_key_x_size) != 0) {
        EMSG("The public key X coordinate does not match the expected verifier.");
        return TEE_ERROR_COMMUNICATION;
    }
    
    if (TEE_MemCompare(ecdsa_service_public_key_y, ctx->msg1.ecdsa_verifier_public_key_y_raw, ecdsa_service_public_key_y_size) != 0) {
        EMSG("The public key Y coordinate does not match the expected verifier.");
        return TEE_ERROR_COMMUNICATION;
    }

    return TEE_SUCCESS;
}

static TEE_Result import_verifier_attestation_key(ra_context *ctx) {
    TEE_Attribute curve_attr;
    TEE_Result res;

    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_PUBLIC_KEY, WASI_RA_ECDSA_KEY_SIZE, &ctx->ecdsa_verifier_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_InitRefAttribute(&ctx->ecdsa_verifier_public_key_x_attr, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg1.ecdsa_verifier_public_key_x_raw, ctx->msg1.ecdsa_verifier_public_key_x_size);
    TEE_InitRefAttribute(&ctx->ecdsa_verifier_public_key_y_attr, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg1.ecdsa_verifier_public_key_y_raw, ctx->msg1.ecdsa_verifier_public_key_y_size);

    TEE_Attribute attributes[] = {ctx->ecdsa_verifier_public_key_x_attr, ctx->ecdsa_verifier_public_key_y_attr, curve_attr};
    res = TEE_PopulateTransientObject(ctx->ecdsa_verifier_public_key_handle, attributes, 3);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        return res;
    }

    DMSG("Public verifier key imported!");

    return TEE_SUCCESS;
}

static TEE_Result verify_ecdh_public_key_signature(ra_context *ctx) {
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle operation_handle;
    uint32_t expected_digest_len = WASI_RA_HASH_SIZE / 8;
	uint32_t digest_len = WASI_RA_HASH_SIZE;
    uint8_t digest[WASI_RA_HASH_SIZE / 8];
    uint32_t expected_sign_len = RA_SIGNATURE_SIZE;
    uint32_t sign_len = RA_SIGNATURE_SIZE;

    // Hashing of the public keys
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    // Gv
    TEE_DigestUpdate(operation_handle, ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
    TEE_DigestUpdate(operation_handle, ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);

    // Ga
    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
    res = TEE_DigestDoFinal(operation_handle, ctx->msg0.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size, digest, &digest_len);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_DigestDoFinal failed. Error: %x", res);
        goto out;
    }

    if (digest_len != expected_digest_len) {
        EMSG("The hash size does not correspond to the expected value (actual: %d; expected: %d).", digest_len, expected_digest_len);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

    TEE_ResetOperation(operation_handle);

    // Verifying of the digest
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_ECDSA_P256, TEE_MODE_VERIFY, WASI_RA_ECDSA_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AllocateOperation OK!");
    
    res = TEE_SetOperationKey(operation_handle, ctx->ecdsa_verifier_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_SetOperationKey OK!");

    res = TEE_AsymmetricVerifyDigest(operation_handle, NULL, 0, digest, digest_len, ctx->msg1.ecdh_signature, sizeof(ctx->msg1.ecdh_signature));
    if (res == TEE_ERROR_SIGNATURE_INVALID) {
        EMSG("The signature of the msg1 is invalid.");
        IMSG("Dumping the msg1:");
        utils_print_byte_array((uint8_t*) &ctx->msg1, sizeof(msg1_t));
        IMSG("Dumping the digest:");
        utils_print_byte_array(digest, digest_len);
        IMSG("Dumping the public key of the verifier (X):");
        utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_x_raw, ctx->msg1.ecdsa_verifier_public_key_x_size);
        IMSG("Dumping the public key of the verifier (Y):");
        utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_y_raw, ctx->msg1.ecdsa_verifier_public_key_y_size);
        IMSG("!!! Details of digest arguments !!!");
        IMSG(" > ctx->msg0.ecdh_local_public_key_x_raw (%u)", ctx->msg0.ecdh_local_public_key_x_size);
        utils_print_byte_array(ctx->msg0.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
        IMSG(" > ctx->msg0.ecdh_local_public_key_y_raw (%u)", ctx->msg0.ecdh_local_public_key_y_size);
        utils_print_byte_array(ctx->msg0.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size);
        IMSG(" > ctx->msg1.ecdh_verifier_public_key_x_raw (%u)", ctx->msg1.ecdh_verifier_public_key_x_size);
        utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
        IMSG(" > ctx->msg1.ecdh_verifier_public_key_y_raw (%u)", ctx->msg1.ecdh_verifier_public_key_y_size);
        utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);
        goto out;
    }
    
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AsymmetricVerifyDigest failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AsymmetricSignDigest OK!");

    if (sign_len != expected_sign_len) {
        EMSG("The signature size does not correspond to the expected value (actual: %d; expected: %d).", sign_len, expected_sign_len);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

    DMSG("The signature of msg1 has been verified!");
    
out:
    TEE_FreeOperation(operation_handle);
    return res;
}

static TEE_Result verify_msg1_mac(ra_context *ctx) {
    TEE_Result res = TEE_SUCCESS;
    uint32_t expected_mac_size = WASI_RA_AES_CMAC_TAG_SIZE / 8;
    
    TEE_OperationHandle aes_cmac_op;

    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, WASI_RA_AES_CMAC_TAG_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        return res;
    }
    DMSG("TEE_AllocateOperation OK!");

    // Set the AES key for the AES-CMAC operation
    res = TEE_SetOperationKey(aes_cmac_op, ctx->ecdh_shared_mac_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        return res;
    }
    DMSG("TEE_SetOperationKey OK!");

    // Compute the tag using AES-CMAC
    TEE_MACInit(aes_cmac_op, NULL, 0);
    DMSG("TEE_MACInit OK!");

    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdsa_verifier_public_key_x_raw, ctx->msg1.ecdsa_verifier_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdsa_verifier_public_key_y_raw, ctx->msg1.ecdsa_verifier_public_key_y_size);

    res = TEE_MACCompareFinal(aes_cmac_op, ctx->msg1.ecdh_signature, RA_SIGNATURE_SIZE, ctx->msg1.mac, expected_mac_size);
    if (res == TEE_ERROR_MAC_INVALID) {
        EMSG("The MAC of the msg1 is invalid. Error: %x", res);
        goto out;
    }
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACCompareFinal failed. Error: %x", res);
        goto out;
    }
    
    DMSG("The MAC of the msg1 is valid!");

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);

    return res;
}

static TEE_Result generate_anchor(ra_context *ctx, uint8_t *anchor, uint32_t anchor_size) {
    TEE_Result res;
	uint32_t digest_len = RA_ANCHOR_SIZE;
    TEE_OperationHandle operation_handle;

    if (anchor_size != RA_ANCHOR_SIZE) {
        EMSG("The anchor buffer must have a size buffer of %u.", RA_ANCHOR_SIZE);
        return TEE_ERROR_SECURITY;
    }

    // Generate the anchor by hashing (G_a, G_v)
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size);
    TEE_DigestUpdate(operation_handle, ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);

    res = TEE_DigestDoFinal(operation_handle, ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size, anchor, &digest_len);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_DigestDoFinal failed. Error: %x", res);
        goto out;
    }

    if (digest_len != RA_ANCHOR_SIZE) {
        EMSG("The hash size of the anchor does not correspond to the expected value (actual: %d; expected: %d).", digest_len, RA_ANCHOR_SIZE);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the digest
    DMSG("Dumping the anchor:");
    utils_print_byte_array(anchor, digest_len);
#endif    

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

static TEE_Result derive_shared_key(ra_context *ctx) {
    TEE_Result res;

    // The shared secret that is the x-coordinate of G_{av}
    TEE_ObjectHandle ecdh_shared_secret_handle;

    // Derive the x-coordinate of G_{av}
    TEE_OperationHandle operation_handle;
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_ECDH_P256, TEE_MODE_DERIVE, WASI_RA_ECDH_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    res = TEE_SetOperationKey(operation_handle, ctx->ecdh_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    res = TEE_AllocateTransientObject(TEE_TYPE_GENERIC_SECRET, WASI_RA_ECDH_KEY_SIZE, &ecdh_shared_secret_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Get the public key of the remote as attributes
    TEE_Attribute public_key[] = {ctx->ecdh_remote_public_key_x_attr, ctx->ecdh_remote_public_key_y_attr};
    TEE_DeriveKey(operation_handle, public_key, 2, ecdh_shared_secret_handle);

    //Store the shared secret into a byte array
    uint8_t shared_secret_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t shared_secret_size = sizeof(shared_secret_raw);

    res = TEE_GetObjectBufferAttribute(ecdh_shared_secret_handle, TEE_ATTR_SECRET_VALUE, shared_secret_raw, &shared_secret_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the shared secret
    DMSG("Dumping the shared secret:");
    utils_print_byte_array(shared_secret_raw, shared_secret_size);
#endif

    // Derive the KDK
    TEE_ResetOperation(operation_handle);
    
    TEE_ObjectHandle aes_cmac_key_handle;
    uint8_t aes_cmac_key_raw[WASI_RA_AES_CMAC_KEY_SIZE / 8] = {0};
    uint32_t aes_cmac_key_size = sizeof(aes_cmac_key_raw);
    TEE_Attribute aes_cmac_key_attr, mac_attr;

    // Create an attribute to store the raw key, which is a byte array filled of zeroes
    TEE_InitRefAttribute(&aes_cmac_key_attr, TEE_ATTR_SECRET_VALUE, aes_cmac_key_raw, sizeof(aes_cmac_key_raw));

    // Allocate the key
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, aes_cmac_key_size * 8, &aes_cmac_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Populate the key
    res = TEE_PopulateTransientObject(aes_cmac_key_handle, &aes_cmac_key_attr, 1);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_AES_CMAC, TEE_MODE_MAC, aes_cmac_key_size * 8);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    // Set the key for the AES-CMAC operation
    res = TEE_SetOperationKey(operation_handle, aes_cmac_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    // Compute the tag using AES-CMAC
    TEE_MACInit(operation_handle, NULL, 0);

    uint8_t mac[WASI_RA_AES_CMAC_TAG_SIZE / 8] = {0};
    uint32_t mac_size = sizeof(mac);
    res = TEE_MACComputeFinal(operation_handle, shared_secret_raw, shared_secret_size, mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }

    // Allocate the object to save the KDK
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, WASI_RA_AES_CMAC_KEY_SIZE, &ctx->ecdh_shared_key_derivation_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Create an attribute from the tag
    TEE_InitRefAttribute(&mac_attr, TEE_ATTR_SECRET_VALUE, mac, mac_size);

    // Populate the KDK with the attribute of the tag
    res = TEE_PopulateTransientObject(ctx->ecdh_shared_key_derivation_key, &mac_attr, 1);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        goto out;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the KDK
    DMSG("Dumping the KDK:");
    util_dump_object_attribute(ctx->ecdh_shared_key_derivation_key, TEE_ATTR_SECRET_VALUE, mac_size);
#endif

out:
    TEE_FreeTransientObject(aes_cmac_key_handle);
    TEE_FreeOperation(operation_handle);
    TEE_FreeTransientObject(ecdh_shared_secret_handle);

    return res;
}

static TEE_Result derive_shared_mac_key(ra_context *ctx, uint8_t *data, uint8_t data_length, TEE_ObjectHandle *output_key) {
    TEE_Result res;
    TEE_Attribute aes_key_attr, mac_attr;
    TEE_ObjectHandle aes_key_handle;
    TEE_OperationHandle aes_cmac_op;

    // Allocate the AES key
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, WASI_RA_AES_CMAC_KEY_SIZE, &aes_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Retrieve the KDK from the ECDH session
    uint8_t shared_secret[32];
    uint32_t shared_secret_size = sizeof(shared_secret);
    res = TEE_GetObjectBufferAttribute(ctx->ecdh_shared_key_derivation_key, TEE_ATTR_SECRET_VALUE, shared_secret, &shared_secret_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        goto out;
    }

    // Create an attribute from the retrieved KDK
    TEE_InitRefAttribute(&aes_key_attr, TEE_ATTR_SECRET_VALUE, shared_secret, shared_secret_size);

    // Populate the AES key using the attribute of the KDK
    res = TEE_PopulateTransientObject(aes_key_handle, &aes_key_attr, 1);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, WASI_RA_AES_CMAC_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    // Set the AES key for the AES-CMAC operation
    res = TEE_SetOperationKey(aes_cmac_op, aes_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    // Compute the tag using AES-CMAC
    TEE_MACInit(aes_cmac_op, NULL, 0);

    uint8_t mac[WASI_RA_AES_CMAC_TAG_SIZE / 8] = {0};
    uint32_t mac_size = sizeof(mac);
    res = TEE_MACComputeFinal(aes_cmac_op, data, data_length, mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }

    // Allocate the object to save the tag as an AES key
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, WASI_RA_AES_CMAC_KEY_SIZE, output_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Create an attribute from the tag
    TEE_InitRefAttribute(&mac_attr, TEE_ATTR_SECRET_VALUE, mac, mac_size);

    // Populate the AES key with the attribute of the tag
    res = TEE_PopulateTransientObject(*output_key, &mac_attr, 1);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("Dumping the AES key derived from AES-CMAC (size: %d):", mac_size);
    util_dump_object_attribute(*output_key, TEE_ATTR_SECRET_VALUE, WASI_RA_AES_CMAC_TAG_SIZE / 8);
#endif

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);
    TEE_FreeTransientObject(aes_key_handle);

    return res;
}

static TEE_Result prepare_msg2(ra_context *ctx, ra_quote *quote) {
    TEE_Result res;
    TEE_OperationHandle aes_cmac_op;

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_MEM_START));
#endif

    // Copy the local ECDH key
    ctx->msg2.ecdh_local_public_key_x_size = ctx->msg0.ecdh_local_public_key_x_size;
    ctx->msg2.ecdh_local_public_key_y_size = ctx->msg0.ecdh_local_public_key_y_size;

    TEE_MemMove(ctx->msg2.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
    TEE_MemMove(ctx->msg2.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size);

    // Copy the quote
    TEE_MemMove(&ctx->msg2.quote, quote, sizeof(ra_quote));

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_START));
#endif

    // Sign the message with AES-CMAC.
    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, WASI_RA_AES_CMAC_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    // Set the AES key for the AES-CMAC operation
    res = TEE_SetOperationKey(aes_cmac_op, ctx->ecdh_shared_mac_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    // Compute the tag using AES-CMAC
    TEE_MACInit(aes_cmac_op, NULL, 0);
    TEE_MACUpdate(aes_cmac_op, ctx->msg2.ecdh_local_public_key_x_raw, ctx->msg0.ecdh_local_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg2.ecdh_local_public_key_y_raw, ctx->msg0.ecdh_local_public_key_y_size);

    uint32_t mac_size = WASI_RA_AES_CMAC_TAG_SIZE / 8;
    res = TEE_MACComputeFinal(aes_cmac_op, &ctx->msg2.quote, sizeof(ra_quote), ctx->msg2.mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("== [+: DUMP MSG2 (struct size: %ld)] ======================== ", sizeof(msg2_t));
    DMSG(" - ecdh_local_public_key_x_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg2.ecdh_local_public_key_x_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_local_public_key_x_size (size: %ld): %u", sizeof(uint32_t), ctx->msg2.ecdh_local_public_key_x_size);
    DMSG(" - ecdh_local_public_key_y_raw (size: %d):", WASI_RA_ECDH_KEY_SIZE / 8);
    utils_print_byte_array(ctx->msg2.ecdh_local_public_key_y_raw, WASI_RA_ECDH_KEY_SIZE / 8);
    DMSG(" - ecdh_local_public_key_y_size (size: %ld): %u", sizeof(uint32_t), ctx->msg2.ecdh_local_public_key_y_size);
    DMSG(" - quote (size: %ld):", sizeof(ra_quote));
    utils_print_byte_array((uint8_t*) &ctx->msg2.quote, sizeof(ra_quote));
    DMSG(" - quote.anchor (size: %d):", RA_ANCHOR_SIZE);
    utils_print_byte_array(ctx->msg2.quote.anchor, RA_ANCHOR_SIZE);
    DMSG(" - quote.version (size: %ld): %u", sizeof(uint32_t), ctx->msg2.quote.version);
    DMSG(" - quote.claim_hash (size: %d):", RA_CLAIM_HASH_SIZE);
    utils_print_byte_array(ctx->msg2.quote.claim_hash, RA_CLAIM_HASH_SIZE);
    DMSG(" - quote.attestation_key (size: %d):", RA_ATTESTATION_KEY_SIZE);
    utils_print_byte_array(ctx->msg2.quote.attestation_key, RA_ATTESTATION_KEY_SIZE);
    DMSG(" - quote.signature (size: %d):", RA_SIGNATURE_SIZE);
    utils_print_byte_array(ctx->msg2.quote.signature, RA_SIGNATURE_SIZE);
    DMSG(" - mac (size: %d):", WASI_RA_AES_CMAC_TAG_SIZE / 8);
    utils_print_byte_array(ctx->msg2.mac, WASI_RA_AES_CMAC_TAG_SIZE / 8);
    DMSG(" - full dump:");
    utils_print_byte_array((uint8_t*) &ctx->msg2, sizeof(msg2_t));
    DMSG("== [-: DUMP MSG2] =========================================== ");
#endif

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_END));
#endif

    return TEE_SUCCESS;
}

static TEE_Result send_msg2(ra_context *ctx) {
    TEE_Result res;
    uint32_t msg2_size = sizeof(msg2_t);

    res = ctx->socket_handle.socket->send(ctx->socket_handle.ctx, &ctx->msg2, &msg2_size, TEE_TIMEOUT_INFINITE);
    if (res != TEE_SUCCESS) {
        EMSG("The msg2 cannot be sent to the verifier. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

static TEE_Result allocate_msg3(ra_context *ctx, uint32_t data_size) {
    // Allocate the msg3 with the size of the data to transfer
    ctx->msg3_size = sizeof(msg3_t) + data_size + WASI_RA_AES_GCM_CIPHERTEXT_OVERHEAD;
    ctx->msg3 = TEE_Malloc(ctx->msg3_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    
    DMSG("allocate_msg3 TEE_Malloc(%u) OK", ctx->msg3_size);

    return TEE_SUCCESS;
}

static TEE_Result receive_msg3(ra_context *ctx) {
    TEE_Result res;

    res = receive_socket_per_chunk(ctx->socket_handle.socket, ctx->socket_handle.ctx, 256 * 1024, (uint8_t*)ctx->msg3, ctx->msg3_size, &ctx->msg3_size);
    if (res != TEE_SUCCESS) {
        EMSG("An error occurred while retrieving the msg3. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    DMSG("msg3 received (buffer size: %u)! Dumping first 30 bytes:", ctx->msg3_size);
    utils_print_byte_array((uint8_t*) ctx->msg3, 30);
#endif

#ifdef DEBUG_MESSAGE
    DMSG("== [+: DUMP MSG3 (struct size: %ld, buffer size: %u)] ======================== ", sizeof(msg3_t), ctx->msg3_size);
    DMSG(" - iv (size: %d):", WASI_RA_AES_GCM_IV_SIZE);
    utils_print_byte_array(ctx->msg3->iv, WASI_RA_AES_GCM_IV_SIZE);
    DMSG(" - tag (size: %d):", WASI_RA_AES_GCM_TAG_SIZE / 8);
    utils_print_byte_array(ctx->msg3->tag, WASI_RA_AES_GCM_TAG_SIZE / 8);
    DMSG(" - encrypted_content_size (size: %ld): %u", sizeof(uint32_t), ctx->msg3->encrypted_content_size);
    DMSG(" - encrypted_content (cropped to first 50 bytes):");
    utils_print_byte_array(ctx->msg3->encrypted_content, 50);
    //DMSG(" - encrypted_content (full):");
    //utils_print_byte_array(ctx->msg3->encrypted_content, ctx->msg3->encrypted_content_size);
    DMSG(" - full dump (cropped to first 50 bytes for encrypted data):");
    utils_print_byte_array((uint8_t*) ctx->msg3, sizeof(msg3_t) + 50);
    //DMSG(" - full dump (full):");
    //utils_print_byte_array((uint8_t*) ctx->msg3, sizeof(msg3_t) + ctx->msg3->encrypted_content_size);
    DMSG("== [-: DUMP MSG3] =========================================== ");
#endif

    return TEE_SUCCESS;
}

static TEE_Result decrypt_msg3(ra_context *ctx, uint8_t *data, uint32_t *data_size) {
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle operation_handle;

    // Decrypt the message with AES-GCM with the session key
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_AES_GCM, TEE_MODE_DECRYPT, WASI_RA_AES_GCM_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    res = TEE_SetOperationKey(operation_handle, ctx->ecdh_shared_session_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    res = TEE_AEInit(operation_handle, ctx->msg3->iv, WASI_RA_AES_GCM_IV_SIZE, WASI_RA_AES_GCM_TAG_SIZE, 0, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AEInit failed. Error: %x", res);
        goto out;
    }

    res = TEE_AEDecryptFinal(operation_handle, ctx->msg3->encrypted_content, ctx->msg3->encrypted_content_size,
            data, data_size, ctx->msg3->tag, WASI_RA_AES_GCM_TAG_SIZE / 8);
    if (res == TEE_ERROR_SHORT_BUFFER) {
        EMSG("The size of the data buffer is too short for the incoming data.");
        goto out;
    }
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AEDecryptFinal failed. Error: %x", res);
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("Dumping first 30 bytes of data of the msg3 (size: %u):", *data_size);
    utils_print_byte_array(data, 30);
#endif

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

static TEE_Result ensure_attestation_service_up(TEE_TASessionHandle *attestation_session) {
    if (*attestation_session != TEE_HANDLE_NULL) return TEE_SUCCESS;

    TEE_UUID uuid = PTA_ATTESTATION_SERVICE_UUID;
    TEE_Result res = TEE_SUCCESS;

	/* Open a session with the Pseudo TA */
	res = TEE_OpenTASession(&uuid, TEE_TIMEOUT_INFINITE, 0, NULL, attestation_session, NULL);
    if (res != TEE_SUCCESS) {
        EMSG("The session with the attestation service cannot be established. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

TEE_Result ra_quote_collect(TEE_TASessionHandle *attestation_session, uint8_t *wasm_bytecode_hash, int wasm_bytecode_hash_size, 
        uint8_t *anchor, int anchor_size, ra_quote *quote) {
    DMSG("has been called");

    TEE_Result res;

    res = ensure_attestation_service_up(attestation_session);
    if (res != TEE_SUCCESS) {
        EMSG("ensure_attestation_service_up failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_QUOTE_START));
#endif

    uint32_t param_types;
	TEE_Param params[TEE_NUM_PARAMS];

	param_types = TEE_PARAM_TYPES(
                    TEE_PARAM_TYPE_MEMREF_INPUT, // claim
                    TEE_PARAM_TYPE_MEMREF_INPUT, // anchor
				    TEE_PARAM_TYPE_MEMREF_INOUT, // quote
				    TEE_PARAM_TYPE_NONE);

    TEE_MemFill(params, 0, sizeof(params));
    params[0].memref.buffer = wasm_bytecode_hash;
    params[0].memref.size = wasm_bytecode_hash_size;
    params[1].memref.buffer = anchor;
    params[1].memref.size = anchor_size;
    params[2].memref.buffer = quote;
    params[2].memref.size = sizeof(ra_quote);
    
    res = TEE_InvokeTACommand(*attestation_session, TEE_TIMEOUT_INFINITE, 
            ATTESTATION_CMD_GEN_QUOTE, param_types, params, NULL);
    if (res != TEE_SUCCESS) {
        EMSG("The attestation service cannot deliver a quote. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_QUOTE_END));
#endif

    return TEE_SUCCESS;
}

TEE_Result ra_quote_dispose(TEE_TASessionHandle *attestation_session, ra_quote *quote) {
    TEE_Result res;

    if ((res = close_attestation_session(attestation_session)) != TEE_SUCCESS) {
        EMSG("The attestation session cannot be closed. Error: %x", res);
        return res;
    }

    TEE_MemFill(quote, 0, sizeof(ra_quote));

    return TEE_SUCCESS;
}

TEE_Result ra_net_handshake(ra_context *ctx, const char* host, uint8_t *ecdsa_service_public_key_x, uint32_t ecdsa_service_public_key_x_size, 
        uint8_t *ecdsa_service_public_key_y, uint32_t ecdsa_service_public_key_y_size, uint8_t *anchor, uint32_t anchor_size) {
    TEE_Result res;
    (void)host;

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_MEM_START));
#endif
    
    if ((res = alloc_ra_context(ctx)) != TEE_SUCCESS) {
        EMSG("alloc_ra_context failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_KEYGEN_START));
#endif

    if ((res = generate_ecdh_keypair(ctx)) != TEE_SUCCESS) {
        EMSG("generate_ecdh_keypair failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_KEYGEN_END));
#endif

    if ((res = establish_tcp_connection(ctx, host)) != TEE_SUCCESS) {
        EMSG("establish_tcp_connection failed. Error: %x", res);
        return res;
    }

    if ((res = send_msg0(ctx)) != TEE_SUCCESS) {
        EMSG("send_msg0 failed. Error: %x", res);
        return res;
    }

    if ((res = receive_msg1(ctx)) != TEE_SUCCESS) {
        EMSG("receive_msg1 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_MEM_START));
#endif

    res = verify_service_public_key(ctx, ecdsa_service_public_key_x, ecdsa_service_public_key_x_size,
            ecdsa_service_public_key_y, ecdsa_service_public_key_y_size);
    if (res != TEE_SUCCESS) {
        EMSG("verify_service_public_key failed. Error: %x", res);
        return res;
    }

    res = import_verifier_attestation_key(ctx);
    if (res != TEE_SUCCESS) {
        EMSG("import_verifier_attestation_key failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_ASYM_CRYPTO_START));
#endif

    if ((res = verify_ecdh_public_key_signature(ctx)) != TEE_SUCCESS) {
        EMSG("verify_ecdh_public_key_signature failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_KEYGEN_START));
#endif

    if ((res = derive_shared_key(ctx)) != TEE_SUCCESS) {
        EMSG("derive_shared_key failed. Error: %x", res);
        return res;
    }

    uint8_t mac_data[] = WASI_RA_SHARED_MAC_DATA;
    res = derive_shared_mac_key(ctx, mac_data, sizeof(mac_data), &ctx->ecdh_shared_mac_key);
    if (res != TEE_SUCCESS) {
        EMSG("derive_shared_mac_key for mac key failed. Error: %x", res);
        return res;
    }

    uint8_t session_data[] = WASI_RA_SHARED_SESSION_DATA;
    res = derive_shared_mac_key(ctx, session_data, sizeof(session_data), &ctx->ecdh_shared_session_key);
    if (res != TEE_SUCCESS) {
        EMSG("derive_shared_mac_key for session key failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_START));
#endif

    if ((res = verify_msg1_mac(ctx)) != TEE_SUCCESS) {
        EMSG("verify_msg1_mac failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_END));
#endif

    res = generate_anchor(ctx, anchor, anchor_size);
    if (res != TEE_SUCCESS) {
        EMSG("generate_anchor failed. Error: %x", res);
        return res;
    }
    return TEE_SUCCESS;
}

TEE_Result ra_net_send_quote(ra_context *ctx, ra_quote *quote) {
    TEE_Result res;

    if ((res = prepare_msg2(ctx, quote)) != TEE_SUCCESS) {
        EMSG("prepare_msg2 failed. Error: %x", res);
        return res;
    }

    if ((res = send_msg2(ctx)) != TEE_SUCCESS) {
        EMSG("send_msg2 failed. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

TEE_Result ra_net_receive_data(ra_context *ctx, uint8_t *data, uint32_t *data_size) {
    TEE_Result res;

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC_START));
#endif

    if ((res = allocate_msg3(ctx, *data_size)) != TEE_SUCCESS) {
        EMSG("allocate_msg3 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC_END));
#endif

    if ((res = receive_msg3(ctx)) != TEE_SUCCESS) {
        EMSG("receive_msg3 failed. Error: %x", res);
        return res;
    }
    DMSG("Received msg3! Size of encrypted content: %u", ctx->msg3->encrypted_content_size);

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_DECRYPT_START));
#endif

    if ((res = decrypt_msg3(ctx, data, data_size)) != TEE_SUCCESS) {
        EMSG("decrypt_msg3 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_DECRYPT_END));
#endif

    return TEE_SUCCESS;
}

TEE_Result ra_net_dispose(ra_context *ctx) {
    return free_ra_context(ctx);
}
