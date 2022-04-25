#include "logging.h"
#include "verifier.h"
#include "tee_benchmarks.h"

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

	IMSG("[%s]", buffer);

	TEE_Free(buffer);
}

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

static TEE_Result generate_ecdh_keypair(ra_context_verifier *ctx) {
    TEE_Attribute curve_attr;
    TEE_Result res;

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_KEYPAIR, RA_ECDH_KEY_SIZE, &ctx->ecdh_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    if ((res = TEE_GenerateKey(ctx->ecdh_local_keypair_handle, RA_ECDH_KEY_SIZE, &curve_attr, 1)) != TEE_SUCCESS) {
        EMSG("TEE_GenerateKey failed. Error: %x", res);
        return res;
    }

    // Extract the public key
    ctx->msg1.ecdh_verifier_public_key_x_size = sizeof(ctx->msg1.ecdh_verifier_public_key_x_raw);
    ctx->msg1.ecdh_verifier_public_key_y_size = sizeof(ctx->msg1.ecdh_verifier_public_key_y_raw);

    res = TEE_GetObjectBufferAttribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg1.ecdh_verifier_public_key_x_raw, &ctx->msg1.ecdh_verifier_public_key_x_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }
    
    res = TEE_GetObjectBufferAttribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg1.ecdh_verifier_public_key_y_raw, &ctx->msg1.ecdh_verifier_public_key_y_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the ECDH keypair
    DMSG("Dumping ECDH TEE_ATTR_ECC_PRIVATE_VALUE:");
    util_dump_object_attribute(ctx->ecdh_local_keypair_handle, TEE_ATTR_ECC_PRIVATE_VALUE, RA_ECDH_KEY_SIZE / 8);
    DMSG("Dumping ECDH TEE_ATTR_ECC_PUBLIC_VALUE_X:");
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
    DMSG("Dumping ECDH TEE_ATTR_ECC_PUBLIC_VALUE_Y:");
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);
#endif

    return TEE_SUCCESS;
}

static TEE_Result store_msg0(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size) {
    if (buffer_size < sizeof(msg0_t)) {
        EMSG("The buffer is too short to contain the msg0.");
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(&ctx->msg0, buffer, sizeof(msg0_t));

#ifdef DEBUG_MESSAGE
    // Dump the public key
    DMSG("Dumping msg0 public key X:");
    utils_print_byte_array(ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg0.ecdh_attester_public_key_x_size);
    DMSG("Dumping msg0 public key Y:");
    utils_print_byte_array(ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg0.ecdh_attester_public_key_y_size);
#endif

    return TEE_SUCCESS;
}

static TEE_Result populate_attester_key(ra_context_verifier *ctx) {
    TEE_Result res;

    // Create the attributes for the public key
    TEE_InitRefAttribute(&ctx->ecdh_attester_public_key_x_attr, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg0.ecdh_attester_public_key_x_size);

    TEE_InitRefAttribute(&ctx->ecdh_attester_public_key_y_attr, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg0.ecdh_attester_public_key_y_size);

    // Define the curve
    TEE_Attribute curve_attr;
    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    // Create the object for the public key
    res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_PUBLIC_KEY, RA_ECDH_KEY_SIZE, &ctx->ecdh_attester_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_Attribute attributes[] = {ctx->ecdh_attester_public_key_x_attr, ctx->ecdh_attester_public_key_y_attr, curve_attr};
    res = TEE_PopulateTransientObject(ctx->ecdh_attester_public_key_handle, attributes, 3);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

static TEE_Result derive_shared_key(ra_context_verifier *ctx) {
    TEE_Result res;

    // The shared secret that is the x-coordinate of G_{av}
    TEE_ObjectHandle ecdh_shared_secret_handle;

    // Derive the x-coordinate of G_{av}
    TEE_OperationHandle operation_handle;
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_ECDH_P256, TEE_MODE_DERIVE, RA_ECDH_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    res = TEE_SetOperationKey(operation_handle, ctx->ecdh_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    res = TEE_AllocateTransientObject(TEE_TYPE_GENERIC_SECRET, RA_ECDH_KEY_SIZE, &ecdh_shared_secret_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        goto out;
    }

    // Get the public key of the attester as attributes
    TEE_Attribute public_key[] = {ctx->ecdh_attester_public_key_x_attr, ctx->ecdh_attester_public_key_y_attr};
    TEE_DeriveKey(operation_handle, public_key, 2, ecdh_shared_secret_handle);

    //Store the shared secret into a byte array
    uint8_t shared_secret_raw[RA_ECDH_KEY_SIZE / 8];
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
    uint8_t aes_cmac_key_raw[RA_AES_CMAC_KEY_SIZE / 8] = {0};
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

    uint8_t mac[RA_AES_CMAC_TAG_SIZE / 8] = {0};
    uint32_t mac_size = sizeof(mac);
    res = TEE_MACComputeFinal(operation_handle, shared_secret_raw, shared_secret_size, mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }

    // Allocate the object to save the KDK
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, RA_AES_CMAC_KEY_SIZE, &ctx->ecdh_shared_key_derivation_key);
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

static TEE_Result derive_shared_mac_key(ra_context_verifier *ctx, uint8_t *data, uint32_t data_length, TEE_ObjectHandle *output_key) {
    TEE_Result res;
    TEE_Attribute aes_key_attr, mac_attr;
    TEE_ObjectHandle aes_key_handle;
    TEE_OperationHandle aes_cmac_op;

    // Allocate the AES key
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, RA_AES_CMAC_KEY_SIZE, &aes_key_handle);
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
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, RA_AES_CMAC_KEY_SIZE);
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

    uint8_t mac[RA_AES_CMAC_TAG_SIZE / 8] = {0};
    uint32_t mac_size = sizeof(mac);
    res = TEE_MACComputeFinal(aes_cmac_op, data, data_length, mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }

    // Allocate the object to save the tag as an AES key
    res = TEE_AllocateTransientObject(TEE_TYPE_AES, RA_AES_CMAC_KEY_SIZE, output_key);
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
    util_dump_object_attribute(*output_key, TEE_ATTR_SECRET_VALUE, RA_AES_CMAC_TAG_SIZE / 8);
#endif

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);
    TEE_FreeTransientObject(aes_key_handle);

    return res;
}

static TEE_Result append_ecdh_public_keys_signature(ra_context_verifier *ctx) {
    TEE_Result res = TEE_SUCCESS;
    TEE_OperationHandle operation_handle;
    uint32_t expected_digest_len = RA_HASH_SIZE / 8;
	uint32_t digest_len = RA_HASH_SIZE;
    uint8_t digest[RA_HASH_SIZE / 8];
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
    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg0.ecdh_attester_public_key_x_size);
    res = TEE_DigestDoFinal(operation_handle, ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg0.ecdh_attester_public_key_y_size, digest, &digest_len);
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
    DMSG("[MSG1] Dumping the digest:");
    utils_print_byte_array(digest, digest_len);
    IMSG("!!! Details of digest arguments !!!");
    IMSG(" > ctx->msg0.ecdh_attester_public_key_x_raw (%u)", ctx->msg0.ecdh_attester_public_key_x_size);
    utils_print_byte_array(ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg0.ecdh_attester_public_key_x_size);
    IMSG(" > ctx->msg0.ecdh_attester_public_key_y_raw (%u)", ctx->msg0.ecdh_attester_public_key_y_size);
    utils_print_byte_array(ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg0.ecdh_attester_public_key_y_size);
    IMSG(" > ctx->msg1.ecdh_verifier_public_key_x_raw (%u)", ctx->msg1.ecdh_verifier_public_key_x_size);
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
    IMSG(" > ctx->msg1.ecdh_verifier_public_key_y_raw (%u)", ctx->msg1.ecdh_verifier_public_key_y_size);
    utils_print_byte_array(ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);
#endif

    TEE_ResetOperation(operation_handle);

    // Signing of the digest
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_ECDSA_P256, TEE_MODE_SIGN, RA_ECDSA_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AllocateOperation OK!");
    
    res = TEE_SetOperationKey(operation_handle, ctx->ecdsa_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_SetOperationKey OK!");

    res = TEE_AsymmetricSignDigest(operation_handle, NULL, 0, digest, digest_len, ctx->msg1.ecdh_signature, &sign_len);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AsymmetricSignDigest failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AsymmetricSignDigest OK!");

    if (sign_len != expected_sign_len) {
        EMSG("The signature size does not correspond to the expected value (actual: %d; expected: %d).", sign_len, expected_sign_len);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("Dumping the signature tag of msg1:");
    utils_print_byte_array(ctx->msg1.ecdh_signature, expected_sign_len);
#endif

out:
    TEE_FreeOperation(operation_handle);
    return res;
}

static TEE_Result append_mac(ra_context_verifier *ctx) {
    TEE_Result res = TEE_SUCCESS;
    uint32_t expected_mac_size = RA_AES_CMAC_TAG_SIZE / 8;
    
    TEE_OperationHandle aes_cmac_op;

    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, RA_AES_CMAC_TAG_SIZE);
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

    uint32_t mac_size = expected_mac_size;

    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdsa_verifier_public_key_x_raw, ctx->msg1.ecdsa_verifier_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg1.ecdsa_verifier_public_key_y_raw, ctx->msg1.ecdsa_verifier_public_key_y_size);

    res = TEE_MACComputeFinal(aes_cmac_op, ctx->msg1.ecdh_signature, RA_SIGNATURE_SIZE, ctx->msg1.mac, &mac_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACComputeFinal failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_MACComputeFinal OK!");

    if (expected_mac_size != mac_size) {
        EMSG("The MAC tag size does not correspond to the expected value (actual: %d; expected: %d).", mac_size, expected_mac_size);
        res = TEE_ERROR_GENERIC;
        goto out;
    }

#ifdef DEBUG_MESSAGE
    DMSG("Dumping the MAC tag of msg1:");
    utils_print_byte_array(ctx->msg1.mac, expected_mac_size);
#endif

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);

    return res;
}

static TEE_Result store_msg2(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size) {
    if (buffer_size < sizeof(msg2_t)) {
        EMSG("The buffer that stores the msg2 is too short compared to the expected size.");
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(&ctx->msg2, buffer, sizeof(msg2_t));

#ifdef DEBUG_MESSAGE
    DMSG("Dumping msg2:");
    utils_print_byte_array((uint8_t*) &ctx->msg2, sizeof(msg2_t) / 2);
    utils_print_byte_array((uint8_t*) &ctx->msg2 + sizeof(msg2_t) / 2, sizeof(msg2_t) / 2);
#endif

    return TEE_SUCCESS;
}

static TEE_Result verify_mac_msg2(ra_context_verifier *ctx) {
    TEE_Result res = TEE_SUCCESS;
    uint32_t expected_mac_size = RA_AES_CMAC_TAG_SIZE / 8;
    
    TEE_OperationHandle aes_cmac_op;

    // Create an operation to perform AES-CMAC
    res = TEE_AllocateOperation(&aes_cmac_op, TEE_ALG_AES_CMAC, TEE_MODE_MAC, RA_AES_CMAC_TAG_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AllocateOperation OK!");

    // Set the AES key for the AES-CMAC operation
    res = TEE_SetOperationKey(aes_cmac_op, ctx->ecdh_shared_mac_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_SetOperationKey OK!");

    // Compute the tag using AES-CMAC
    TEE_MACInit(aes_cmac_op, NULL, 0);
    DMSG("TEE_MACInit OK!");

    TEE_MACUpdate(aes_cmac_op, ctx->msg2.ecdh_local_public_key_x_raw, ctx->msg2.ecdh_local_public_key_x_size);
    TEE_MACUpdate(aes_cmac_op, ctx->msg2.ecdh_local_public_key_y_raw, ctx->msg2.ecdh_local_public_key_y_size);

    res = TEE_MACCompareFinal(aes_cmac_op, &ctx->msg2.quote, sizeof(ra_quote), ctx->msg2.mac, expected_mac_size);
    if (res == TEE_ERROR_MAC_INVALID) {
        EMSG("The MAC of the msg2 is invalid. Error: %x", res);
        goto out;
    }
    if (res != TEE_SUCCESS) {
        EMSG("TEE_MACCompareFinal failed. Error: %x", res);
        goto out;
    }
    DMSG("The MAC of the msg2 is valid!");

out:
    // Free the local resource
    TEE_FreeOperation(aes_cmac_op);

    return res;
}

static TEE_Result verify_public_key_msg2_matches_msg0(ra_context_verifier *ctx) {
    int compare_result;

    compare_result = TEE_MemCompare(ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg2.ecdh_local_public_key_x_raw, \
        ctx->msg0.ecdh_attester_public_key_x_size);
    if (compare_result != 0) {
        EMSG("The public key of msg0 is not equal to msg2 (X-coordinate).");
        return TEE_ERROR_SECURITY;
    }

    compare_result = TEE_MemCompare(ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg2.ecdh_local_public_key_y_raw, \
        ctx->msg0.ecdh_attester_public_key_y_size);
    if (compare_result != 0) {
        EMSG("The public key of msg0 is not equal to msg2 (X-coordinate).");
        return TEE_ERROR_SECURITY;
    }

    DMSG("The public key of msg0 and msg2 are equal!");

    return TEE_SUCCESS;
}

static TEE_Result verify_quote_signature(ra_context_verifier *ctx) {
    TEE_Result res;
    TEE_OperationHandle operation_handle;
    uint32_t expected_digest_len = RA_HASH_SIZE / 8;
	uint32_t digest_len = RA_HASH_SIZE;
    uint8_t digest[RA_HASH_SIZE / 8];

    // Hashing of the quote
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    TEE_DigestUpdate(operation_handle, ctx->msg2.quote.anchor, sizeof(ctx->msg2.quote.anchor));
    TEE_DigestUpdate(operation_handle, &ctx->msg2.quote.version, sizeof(ctx->msg2.quote.version));
    TEE_DigestUpdate(operation_handle, ctx->msg2.quote.claim_hash, sizeof(ctx->msg2.quote.claim_hash));
    DMSG("TEE_DigestUpdate OK!");

    res = TEE_DigestDoFinal(operation_handle, ctx->msg2.quote.attestation_key, sizeof(ctx->msg2.quote.attestation_key), digest, &digest_len);
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

    // Verify the signature of the quote
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_ECDSA_P256, TEE_MODE_VERIFY, RA_ECDSA_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }
    
    res = TEE_SetOperationKey(operation_handle, ctx->ecdsa_attester_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    res = TEE_AsymmetricVerifyDigest(operation_handle, NULL, 0, digest, digest_len, ctx->msg2.quote.signature, sizeof(ctx->msg2.quote.signature));
    if (res == TEE_ERROR_SIGNATURE_INVALID) {
        EMSG("The signature of the quote is invalid.");
        goto out;
    }

    if (res != TEE_SUCCESS) {
        EMSG("TEE_AsymmetricVerifyDigest failed. Error: %x", res);
        goto out;
    }

    DMSG("The quote is valid!");
    ctx->is_quote_valid = true;

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

static TEE_Result verify_anchor(ra_context_verifier *ctx) {
    TEE_Result res;
    uint8_t digest[RA_ANCHOR_SIZE];
	uint32_t digest_len = RA_ANCHOR_SIZE;
    TEE_OperationHandle operation_handle;

    // Generate the anchor by hashing (G_a, G_v)
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_attester_public_key_x_raw, ctx->msg0.ecdh_attester_public_key_x_size);
    TEE_DigestUpdate(operation_handle, ctx->msg0.ecdh_attester_public_key_y_raw, ctx->msg0.ecdh_attester_public_key_y_size);
    TEE_DigestUpdate(operation_handle, ctx->msg1.ecdh_verifier_public_key_x_raw, ctx->msg1.ecdh_verifier_public_key_x_size);

    res = TEE_DigestDoFinal(operation_handle, ctx->msg1.ecdh_verifier_public_key_y_raw, ctx->msg1.ecdh_verifier_public_key_y_size, &digest, &digest_len);
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
    DMSG("Dumping the received anchor:");
    utils_print_byte_array(ctx->msg2.quote.anchor, digest_len);
    DMSG("Dumping the computed anchor:");
    utils_print_byte_array(digest, digest_len);
#endif  

    // Compare the computed hash with the received one
    if (TEE_MemCompare(ctx->msg2.quote.anchor, digest, digest_len) != 0) {
        EMSG("The public keys cannot be verified.");
        return TEE_ERROR_SECURITY;
    }

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

static TEE_Result allocate_msg3(ra_context_verifier *ctx, uint32_t buffer_size) {
    // Allocate the header of msg3; the data are directly encrypted in the shared memory
    ctx->msg3_size = sizeof(msg3_t);
    if (ctx->msg3_size > buffer_size) {
       EMSG("The provided buffer is too small to store the msg3.");
       return TEE_ERROR_SHORT_BUFFER;
    }

    ctx->msg3 = TEE_Malloc(ctx->msg3_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
    DMSG("allocate_msg3 TEE_Malloc OK (%u)", ctx->msg3_size);

    return TEE_SUCCESS;
}

static TEE_Result encrypt_msg3(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size, uint8_t *data, uint32_t data_size) {
    TEE_Result res;
    uint32_t tag_size;

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_ENCRYPT_START));
#endif

    // Generate a iv for AES_GCM
    TEE_GenerateRandom(ctx->msg3->iv, RA_AES_GCM_IV_SIZE);
    DMSG("TEE_GenerateRandom OK!");

    // Encrypt the message with AES-GCM with the session key
    TEE_OperationHandle operation_handle;
    res = TEE_AllocateOperation(&operation_handle, TEE_ALG_AES_GCM, TEE_MODE_ENCRYPT, RA_AES_GCM_KEY_SIZE);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateOperation failed. Error: %x", res);
        goto out;
    }

    res = TEE_SetOperationKey(operation_handle, ctx->ecdh_shared_session_key);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_SetOperationKey failed. Error: %x", res);
        goto out;
    }

    res = TEE_AEInit(operation_handle, ctx->msg3->iv, RA_AES_GCM_IV_SIZE, RA_AES_GCM_TAG_SIZE, 0, 0);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AEInit failed. Error: %x", res);
        goto out;
    }

    ctx->msg3->encrypted_content_size = buffer_size - sizeof(msg3_t);
    tag_size = sizeof(ctx->msg3->tag);

    // The encrypted data is written directly in the buffer of the shared memory.
    res = TEE_AEEncryptFinal(operation_handle, data, data_size, buffer + sizeof(msg3_t),
            &ctx->msg3->encrypted_content_size, ctx->msg3->tag, &tag_size);
    if (res == TEE_ERROR_SHORT_BUFFER) {
        EMSG("TEE_AEEncryptFinal failed because the buffer was too short to store the data. Error: %x", res);
        goto out;
    }
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AEEncryptFinal failed. Error: %x", res);
        goto out;
    }
    DMSG("TEE_AEEncryptFinal finished! Size of encrypted content: %u", ctx->msg3->encrypted_content_size);

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_ENCRYPT_END));
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC2_START));
#endif

    // Copy the header of the message 3
    TEE_MemMove(buffer, ctx->msg3, sizeof(msg3_t));

out:
    TEE_FreeOperation(operation_handle);

    return res;
}

static TEE_Result free_msg3(ra_context_verifier *ctx) {
    TEE_Free(ctx->msg3);

    return TEE_SUCCESS;
}

TEE_Result import_attester_attestation_key(ra_context_verifier *ctx) {
    uint8_t ecdsa_attester_public_key_x_raw[] = RA_IMX_TEST_PUBLIC_KEY_X;
    uint8_t ecdsa_attester_public_key_y_raw[] = RA_IMX_TEST_PUBLIC_KEY_Y;
    TEE_Attribute curve_attr;
    TEE_Result res;

    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_PUBLIC_KEY, RA_ECDSA_KEY_SIZE, &ctx->ecdsa_attester_public_key_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_InitRefAttribute(&ctx->ecdsa_attester_public_key_x_attr, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ecdsa_attester_public_key_x_raw, sizeof(ecdsa_attester_public_key_x_raw));
    TEE_InitRefAttribute(&ctx->ecdsa_attester_public_key_y_attr, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ecdsa_attester_public_key_y_raw, sizeof(ecdsa_attester_public_key_y_raw));

    TEE_Attribute attributes[] = {ctx->ecdsa_attester_public_key_x_attr, ctx->ecdsa_attester_public_key_y_attr, curve_attr};
    res = TEE_PopulateTransientObject(ctx->ecdsa_attester_public_key_handle, attributes, 3);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_PopulateTransientObject failed. Error: %x", res);
        return res;
    }

    DMSG("Public attestation key imported!");

    return TEE_SUCCESS;
}

TEE_Result generate_ecdsa_keypair(ra_context_verifier *ctx) {
    TEE_Attribute curve_attr;
    TEE_Result res;

    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR, RA_ECDSA_KEY_SIZE, &ctx->ecdsa_local_keypair_handle);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_AllocateTransientObject failed. Error: %x", res);
        return res;
    }

    TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, sizeof(int));

    if ((res = TEE_GenerateKey(ctx->ecdsa_local_keypair_handle, RA_ECDSA_KEY_SIZE, &curve_attr, 1)) != TEE_SUCCESS) {
        EMSG("TEE_GenerateKey failed. Error: %x", res);
        return res;
    }

    // Extract the public key
    ctx->msg1.ecdsa_verifier_public_key_x_size = sizeof(ctx->msg1.ecdsa_verifier_public_key_x_raw);
    ctx->msg1.ecdsa_verifier_public_key_y_size = sizeof(ctx->msg1.ecdsa_verifier_public_key_y_raw);

    res = TEE_GetObjectBufferAttribute(ctx->ecdsa_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_X, 
            ctx->msg1.ecdsa_verifier_public_key_x_raw, &ctx->msg1.ecdsa_verifier_public_key_x_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }
    
    res = TEE_GetObjectBufferAttribute(ctx->ecdsa_local_keypair_handle, TEE_ATTR_ECC_PUBLIC_VALUE_Y, 
            ctx->msg1.ecdsa_verifier_public_key_y_raw, &ctx->msg1.ecdsa_verifier_public_key_y_size);
    if (res != TEE_SUCCESS) {
        EMSG("TEE_GetObjectBufferAttribute failed. Error: %x", res);
        return res;
    }

#ifdef DEBUG_MESSAGE
    // Dumping the ECDSA keypair
    DMSG("Dumping ECDSA TEE_ATTR_ECC_PRIVATE_VALUE:");
    util_dump_object_attribute(ctx->ecdsa_local_keypair_handle, TEE_ATTR_ECC_PRIVATE_VALUE, RA_ECDSA_KEY_SIZE / 8);
    DMSG("Dumping ECDSA TEE_ATTR_ECC_PUBLIC_VALUE_X (%u):", ctx->msg1.ecdsa_verifier_public_key_x_size);
    utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_x_raw, ctx->msg1.ecdsa_verifier_public_key_x_size);
    DMSG("Dumping ECDSA TEE_ATTR_ECC_PUBLIC_VALUE_Y (%u):", ctx->msg1.ecdsa_verifier_public_key_y_size);
    utils_print_byte_array(ctx->msg1.ecdsa_verifier_public_key_y_raw, ctx->msg1.ecdsa_verifier_public_key_y_size);
#endif

    return TEE_SUCCESS;
}

TEE_Result handle_msg0(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size) {
    TEE_Result res;

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_KEYGEN1_START));
#endif

    if ((res = generate_ecdh_keypair(ctx)) != TEE_SUCCESS) {
        EMSG("generate_ecdh_keypair failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_MEM_START));
#endif

    if ((res = store_msg0(ctx, buffer, buffer_size)) != TEE_SUCCESS) {
        EMSG("store_msg0 failed. Error: %x", res);
        return res;
    }

    if ((res = populate_attester_key(ctx)) != TEE_SUCCESS) {
        EMSG("populate_attester_key failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_KEYGEN2_START));
#endif

    if ((res = derive_shared_key(ctx)) != TEE_SUCCESS) {
        EMSG("derive_shared_key failed. Error: %x", res);
        return res;
    }

    DMSG("Deriving Km..");
    uint8_t mac_data[] = RA_SHARED_MAC_DATA;
    res = derive_shared_mac_key(ctx, mac_data, sizeof(mac_data), &ctx->ecdh_shared_mac_key);
    if (res != TEE_SUCCESS) {
        EMSG("derive_shared_mac_key for mac key failed. Error: %x", res);
        return res;
    }

    DMSG("Deriving Ke..");
    uint8_t session_data[] = RA_SHARED_SESSION_DATA;
    res = derive_shared_mac_key(ctx, session_data, sizeof(session_data), &ctx->ecdh_shared_session_key);
    if (res != TEE_SUCCESS) {
        EMSG("derive_shared_mac_key for session key failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE0_KEYGEN2_END));
#endif

    return TEE_SUCCESS;
}

TEE_Result prepare_msg1(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size, uint32_t *msg1_size) {
    TEE_Result res;
    DMSG("has been called");

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_MEM1_START));
#endif

    if (buffer_size < sizeof(msg1_t)) {
        EMSG("the buffer is too short to contain msg1. (expected: %ld; actual: %u)", sizeof(msg1_t), buffer_size);
        return TEE_ERROR_SHORT_BUFFER;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_ASYM_CRYPTO_START));
#endif

    if ((res = append_ecdh_public_keys_signature(ctx)) != TEE_SUCCESS) {
        EMSG("append_ecdh_public_keys_signature failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_START));
#endif

    if ((res = append_mac(ctx)) != TEE_SUCCESS) {
        EMSG("append_mac failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_MEM2_START));
#endif

    // Serialize the msg1 into the buffer
    TEE_MemMove(buffer, &ctx->msg1, sizeof(msg1_t));
    *msg1_size = sizeof(msg1_t);

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE1_MEM2_END));
#endif

#ifdef DEBUG_MESSAGE
    DMSG("msg1 prepared (struct size: %ld)! Dumping:", sizeof(msg1_t));
    utils_print_byte_array(buffer, sizeof(msg1_t) / 2);
    utils_print_byte_array(buffer + sizeof(msg1_t) / 2, sizeof(msg1_t) / 2);
#endif

    return TEE_SUCCESS;
}

TEE_Result handle_msg2(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size) {
    TEE_Result res;

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_MEM1_START));
#endif

    ctx->is_quote_valid = false;

    if ((res = store_msg2(ctx, buffer, buffer_size)) != TEE_SUCCESS) {
        EMSG("store_msg2 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_START));
#endif

    if ((res = verify_mac_msg2(ctx)) != TEE_SUCCESS) {
        EMSG("verify_mac_msg2 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_MEM2_START));
#endif

    if ((res = verify_public_key_msg2_matches_msg0(ctx)) != TEE_SUCCESS) {
        EMSG("verify_public_key_msg2_matches_msg0 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_START));
#endif

    if ((res = verify_quote_signature(ctx)) != TEE_SUCCESS) {
        EMSG("verify_public_key_msg2_matches_msg0 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGES
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_END));
#endif

    if ((res = verify_anchor(ctx)) != TEE_SUCCESS) {
        EMSG("verify_anchor failed. Error: %x", res);
        return res;
    }

    return TEE_SUCCESS;
}

TEE_Result prepare_msg3(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size, uint8_t *data, uint32_t data_size,
        void *benchmark_buffer, uint32_t benchmark_buffer_size) {
    (void)&benchmark_buffer;
    (void)&benchmark_buffer_size;

    TEE_Result res;

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC1_START));
#endif

    if ((res = allocate_msg3(ctx, buffer_size)) != TEE_SUCCESS) {
        EMSG("allocate_msg3 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC1_END));
#endif

    if ((res = encrypt_msg3(ctx, buffer, buffer_size, data, data_size)) != TEE_SUCCESS) {
        EMSG("encrypt_msg3 failed. Error: %x", res);
        return res;
    }
    DMSG("encrypt_msg3 OK");

    if ((res = free_msg3(ctx)) != TEE_SUCCESS) {
        EMSG("encrypt_msg3 failed. Error: %x", res);
        return res;
    }

#ifdef PROFILING_MESSAGE3
    TEE_GetREETime(benchmark_get_store(PROFILING_MESSAGE3_MALLOC2_END));

    snprintf(benchmark_buffer, benchmark_buffer_size, "%ld,%ld,%ld,%ld,%ld,%ld\n", \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC1_START), \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC1_END), \
            benchmark_get_value(PROFILING_MESSAGE3_ENCRYPT_START), \
            benchmark_get_value(PROFILING_MESSAGE3_ENCRYPT_END), \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC2_START), \
            benchmark_get_value(PROFILING_MESSAGE3_MALLOC2_END));
#endif

#ifdef DEBUG_MESSAGE
    DMSG("msg3 prepared (buffer size: %u)! Dumping first 30 bytes:", buffer_size);
    utils_print_byte_array(buffer, 30);
#endif

    return TEE_SUCCESS;
}

TEE_Result dispose_ra_context(ra_context_verifier *ctx) {
    (void)&ctx;

    TEE_Free(ctx->secret);

    TEE_FreeTransientObject(ctx->ecdsa_attester_public_key_handle);
    TEE_FreeTransientObject(ctx->ecdh_attester_public_key_handle);
    TEE_FreeTransientObject(ctx->ecdsa_local_keypair_handle);
    TEE_FreeTransientObject(ctx->ecdh_local_keypair_handle);
    TEE_FreeTransientObject(ctx->ecdh_shared_key_derivation_key);
    TEE_FreeTransientObject(ctx->ecdh_shared_mac_key);
    TEE_FreeTransientObject(ctx->ecdh_shared_session_key);

    return TEE_SUCCESS;
}