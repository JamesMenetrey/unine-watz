/*
 * Copyright (c) 2021, University of Neuchâtel
 *
 * Pseudo TA: Attestation Service
 */
#include <compiler.h>
#include <stdio.h>
#include <trace.h>
#include <kernel/huk_subkey.h>
#include <kernel/pseudo_ta.h>
#include <mm/tee_pager.h>
#include <mm/tee_mm.h>
#include <string.h>
#include <string_ext.h>
#include <malloc.h>
#include <crypto/crypto.h>
#include <kernel/tee_common_otp.h>

#include "attestation.h"
#include "pta_attestation_service.h"
#include "vedliot_crypto.h"
#include "vedliot_utils.h"

#define PTA_NAME "attestation_service.pta"

#define UINT8_DIGIT_MAX_SIZE 		2
#define ATTESTATION_SEED_SIZE		HUK_SUBKEY_MAX_LEN
#define SIGNATURE_ALGORITHM			TEE_ALG_ECDSA_P256
#define SIGNATURE_KEY_TYPE			TEE_TYPE_ECDSA_KEYPAIR
#define SIGNATURE_CURVE				TEE_ECC_CURVE_NIST_P256
#define SIGNATURE_CURVE_BIT_LENGTH	256
#define LTC_KEYPAIR_BUFFER_SIZE		250

// Attestation seed (the value used for key derivation)
static bool is_attestation_seed_derived = false;
static uint8_t attestation_seed[ATTESTATION_SEED_SIZE];

// The attestation keypairs as the abstraction layer type
static bool is_attestation_ecdsa_keypair_derived = false;
static struct ecc_keypair attestation_ecdsa_keypair;

// The attestation public key as an exported blob of bytes
static uint8_t ltc_attestation_exported_public_key[RA_ATTESTATION_KEY_SIZE];
static uint64_t ltc_attestation_exported_public_key_size;


/*
 * Pseudo Trusted Application debug functions
 */

#if TRACE_LEVEL >= TRACE_DEBUG
static void debug_print_huk(void)
{
	TEE_Result res;
	struct tee_hw_unique_key huk;
	memset(&huk, 0, sizeof(struct tee_hw_unique_key));

	if ((res = tee_otp_get_hw_unique_key(&huk)) != TEE_SUCCESS) {
		EMSG("The hardware unique key cannot be obtained. Error: %x", res);
		return;
	}

	DMSG("Dumping the hardware unique key:");
	utils_print_byte_array(huk.data, sizeof(huk.data));
}

static void debug_print_attestation_seed(void)
{
	DMSG("Dumping the attestation seed:");
	utils_print_byte_array(attestation_seed, ATTESTATION_SEED_SIZE);
}
#endif



/*
 * Pseudo Trusted Application internal functions
 */

static TEE_Result hash_byte_array(uint8_t *dst, uint8_t *src, size_t src_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	void *ctx = NULL;
	size_t digest_len = RA_CLAIM_HASH_SIZE;

	res = crypto_hash_alloc_ctx(&ctx, RA_CLAIM_HASH_ALGO);
	if (res)
		goto err;

	res = crypto_hash_init(ctx);
	if (res)
		goto err;

	res = crypto_hash_update(ctx, src, src_len);
	if (res)
		goto err;

	res = crypto_hash_final(ctx, dst, digest_len);
	if (res)
		goto err;

	res = TEE_SUCCESS;
err:
	crypto_hash_free_ctx(ctx);
	return res;
}

static TEE_Result hash_quote(uint8_t *dst, ra_quote *quote)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	void *ctx = NULL;
	size_t digest_len = RA_QUOTE_HASH_SIZE;

	res = crypto_hash_alloc_ctx(&ctx, RA_QUOTE_HASH_ALGO);
	if (res)
		goto err;

	res = crypto_hash_init(ctx);
	if (res)
		goto err;

	// Hash the fields
	res = crypto_hash_update(ctx, quote->anchor, RA_ANCHOR_SIZE);
	if (res)
		goto err;

	res = crypto_hash_update(ctx, (uint8_t*) &quote->version, sizeof(uint32_t));
	if (res)
		goto err;

	res = crypto_hash_update(ctx, quote->claim_hash, RA_CLAIM_HASH_SIZE);
	if (res)
		goto err;

	res = crypto_hash_update(ctx, quote->attestation_key, RA_ATTESTATION_KEY_SIZE);
	if (res)
		goto err;

	res = crypto_hash_final(ctx, dst, digest_len);
	if (res)
		goto err;

	res = TEE_SUCCESS;
err:
	crypto_hash_free_ctx(ctx);
	return res;
}

static TEE_Result derive_attestation_seed(void)
{
    DMSG("has been called");
	TEE_Result res;

	if (is_attestation_seed_derived) return TEE_SUCCESS;

	res = huk_subkey_derive(HUK_SUBKEY_SE050, NULL, 0, (uint8_t*)&attestation_seed, ATTESTATION_SEED_SIZE); 

	if (res == TEE_SUCCESS) is_attestation_seed_derived = true;

#if TRACE_LEVEL >= TRACE_DEBUG
	debug_print_attestation_seed();
#endif

	return res;
}

static TEE_Result derive_attestation_ecdsa_keypair(void)
{
    DMSG("has been called");
	TEE_Result res;

	if (is_attestation_ecdsa_keypair_derived) return TEE_SUCCESS;

	res = crypto_acipher_alloc_ecc_keypair(&attestation_ecdsa_keypair, SIGNATURE_KEY_TYPE, SIGNATURE_CURVE_BIT_LENGTH);
	if (res != TEE_SUCCESS) {
		EMSG("The attestation keypair cannot be allocated. Error: %x", res);
		return res;
	}
	DMSG("crypto_acipher_alloc_ecc_keypair OK");

	// Set the curve
	attestation_ecdsa_keypair.curve = SIGNATURE_CURVE;

	res = gen_ecdsa_key_with_seed(attestation_seed, ATTESTATION_SEED_SIZE, &attestation_ecdsa_keypair, SIGNATURE_CURVE_BIT_LENGTH);
	if (res != TEE_SUCCESS) {
		EMSG("The attestation keypair cannot be derived. Error: %x", res);
		return res;
	}
	DMSG("gen_ecdsa_key_with_seed OK");

	// Export the public key for future quotes
	ltc_attestation_exported_public_key_size = RA_ATTESTATION_KEY_SIZE;
	res = ecc_export_public_in_buffer(&attestation_ecdsa_keypair, SIGNATURE_ALGORITHM,
			ltc_attestation_exported_public_key, &ltc_attestation_exported_public_key_size);
	if (res != TEE_SUCCESS) {
		EMSG("The LTC attestation public key cannot be exported. Error: %x", res);
		return res;
	}
	DMSG("ecc_export_public_in_buffer OK");

#if TRACE_LEVEL >= TRACE_DEBUG
	DMSG("The LTC attestation public key has been exported. blob size: %ld. Dumping:", ltc_attestation_exported_public_key_size);
	utils_print_byte_array(ltc_attestation_exported_public_key, ltc_attestation_exported_public_key_size);
#endif

	if (res == TEE_SUCCESS) is_attestation_ecdsa_keypair_derived = true;

	return res;
}

static TEE_Result generate_quote(uint8_t *claim, uint32_t claim_size, uint8_t *anchor, uint32_t anchor_size, ra_quote *quote)
{
	DMSG("has been called");
	TEE_Result res;

	if (anchor_size > RA_ANCHOR_SIZE) return TEE_ERROR_SHORT_BUFFER;

	// Initialize the quote
	memset(quote, 0, sizeof(ra_quote));
	quote->version = RA_CURRENT_VERSION;
	memcpy(quote->anchor, anchor, RA_ANCHOR_SIZE);
	memcpy(quote->attestation_key, ltc_attestation_exported_public_key, RA_ATTESTATION_KEY_SIZE);

	// Check whether the claim is already a hash based on its size
	if (claim_size == RA_CLAIM_HASH_SIZE) {
		DMSG("Reusing the claim as a hash.");

		// Copy the claim as the hash
		memcpy(quote->claim_hash, claim, claim_size);
	}
	else
	{
		// Hash the claim
		if ((res = hash_byte_array(quote->claim_hash, claim, claim_size)) != TEE_SUCCESS) {
			EMSG("The claim cannot be hashed. Error: %x", res);
			return res;
		}
	}

	// Hash the field of the quote
	uint8_t quote_hash[RA_QUOTE_HASH_SIZE];
	if ((res = hash_quote(quote_hash, quote)) != TEE_SUCCESS) {
		EMSG("The quote cannot be hashed. Error: %x", res);
		return res;
	}

	// Sign the hash of the quote
	size_t sign_len = RA_SIGNATURE_SIZE;
	res = crypto_acipher_ecc_sign(RA_SIGNATURE_ALGO, &attestation_ecdsa_keypair, quote_hash, RA_QUOTE_HASH_SIZE,
			quote->signature, &sign_len);
	if (res != TEE_SUCCESS) {
		EMSG("The quote cannot be signed. Error: %x", res);
		return res;
	}

#if TRACE_LEVEL >= TRACE_DEBUG
	DMSG("Dumping the signature:");
	utils_print_byte_array(quote->signature, RA_SIGNATURE_SIZE);
#endif

	if (RA_SIGNATURE_SIZE != sign_len) {
		EMSG("The signature size does not match. Actual: %ld", sign_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;
}


/*
 * Pseudo Trusted Application test functions
 */

static void test_ecdsa_sign_and_verify(void)
{
	TEE_Result res;
	const uint8_t msg[] = "hello, world!";
	uint8_t signature[150];
	size_t signature_len; 
	memset(signature, 0, sizeof(signature));

	res = crypto_acipher_ecc_sign(SIGNATURE_ALGORITHM, &attestation_ecdsa_keypair, msg, sizeof(msg), signature, &signature_len);
	if (res != TEE_SUCCESS) {
		EMSG("[TEST] The message cannot be signed with the ECDSA keypair. Error: %x", res);
		return;
	}

	DMSG("[TEST] Dumping the signature of '%s':", msg);
	utils_print_byte_array(signature, signature_len);

	struct ecc_public_key key_public;
	res = crypto_acipher_alloc_ecc_public_key(&key_public, TEE_TYPE_ECDSA_PUBLIC_KEY, SIGNATURE_CURVE_BIT_LENGTH);
	if (res != TEE_SUCCESS) {
		EMSG("[TEST] The public key of the ECDA keypair cannot be sallocated. Error: %x", res);
		return;
	}

	// Manual reference of the public key of the keypair
	struct bignum *public_x = key_public.x;
	struct bignum *public_y = key_public.y;
	key_public.x = attestation_ecdsa_keypair.x;
	key_public.y = attestation_ecdsa_keypair.y;
	key_public.curve = SIGNATURE_CURVE;

	res = crypto_acipher_ecc_verify(SIGNATURE_ALGORITHM, &key_public, msg, sizeof(msg), signature, signature_len);
	if (res != TEE_SUCCESS) {
		EMSG("[TEST] The signature cannot be verified on a valid signature. Error: %x", res);
		return;
	}

	DMSG("[TEST] The signature of the message '%s' has been verified!", msg);

	// Change the signature to make it invalid and validate it cannot be verified
	signature[0] += 1;
	res = crypto_acipher_ecc_verify(SIGNATURE_ALGORITHM, &key_public, msg, sizeof(msg), signature, signature_len);

	// Restore the reference of the public key
	key_public.x = public_x;
	key_public.y = public_y;

	// Free the public key
	crypto_acipher_free_ecc_public_key(&key_public);

	if (res != TEE_ERROR_SIGNATURE_INVALID) {
		EMSG("[TEST] An altered signature as been wrongly identified as valid. Error: %x", res);
		return;
	}

	DMSG("[TEST] An altered signature has been correctly identified as faulty!");
}

static void test_gen_quote(void) {
	TEE_Result res;
	uint8_t msg[] = "hello, world!";
	uint8_t anchor[RA_ANCHOR_SIZE] = "hello";
	ra_quote quote;

	//
	// 1. Generate the quote
	//
	if ((res = generate_quote(msg, sizeof(msg), anchor, RA_ANCHOR_SIZE, &quote)) != TEE_SUCCESS) {
		EMSG("[TEST] Cannot generate a quote. Error: %x", res);
		return;
	}

	DMSG("[TEST] The quote has been generated with success! Message: '%s'. Dumping quote:", msg);
	utils_print_byte_array((uint8_t*) &quote, sizeof(quote));

	//
	// 2. Hash the quote
	//
	uint8_t quote_hash[RA_QUOTE_HASH_SIZE];

	if ((res = hash_quote(quote_hash, &quote)) != TEE_SUCCESS) {
		EMSG("[TEST] Cannot hash the quote. Error: %x", res);
		return;
	}

	//
	// 3. Verify the signature of the quote with the public key
	//
	struct ecc_public_key key_public;
	res = crypto_acipher_alloc_ecc_public_key(&key_public, TEE_TYPE_ECDSA_PUBLIC_KEY, SIGNATURE_CURVE_BIT_LENGTH);
	if (res != TEE_SUCCESS) {
		EMSG("[TEST] The public key of the ECDA keypair cannot be sallocated. Error: %x", res);
		return;
	}

	// Manual reference of the public key of the keypair
	struct bignum *public_x = key_public.x;
	struct bignum *public_y = key_public.y;
	key_public.x = attestation_ecdsa_keypair.x;
	key_public.y = attestation_ecdsa_keypair.y;
	key_public.curve = SIGNATURE_CURVE;

	res = crypto_acipher_ecc_verify(RA_SIGNATURE_ALGO, &key_public, quote_hash, RA_QUOTE_HASH_SIZE,
			quote.signature, RA_SIGNATURE_SIZE);

	// Restore the reference of the public key
	key_public.x = public_x;
	key_public.y = public_y;

	// Free the public key
	crypto_acipher_free_ecc_public_key(&key_public);

	if (res != TEE_SUCCESS) {
		EMSG("[TEST] The signature of the quote could not be verified. Error: %x", res);
		return;
	}

	IMSG("[TEST] The signature of the quote has been verified!");
}


/*
 * Pseudo Trusted Application Entry Points
 */

static TEE_Result create(void)
{
    DMSG("has been called");
	TEE_Result res;

#if TRACE_LEVEL >= TRACE_DEBUG
	debug_print_huk();
#endif

	if ((res = derive_attestation_seed()) != TEE_SUCCESS) {
		EMSG("The attestation seed cannot be derived. Error: %x", res);
		return res;
	}

	if ((res = derive_attestation_ecdsa_keypair()) != TEE_SUCCESS) {
		EMSG("The attestation ECDSA keys cannot be derived. Error: %x", res);
		return res;
	}
	
	return res;
}

static TEE_Result invoke_command(void *psess,
				 uint32_t cmd, uint32_t types,
				 TEE_Param params[TEE_NUM_PARAMS])
{
    DMSG("has been called");
	(void)&psess;
	(void)&params;

	switch (cmd) {
	case ATTESTATION_CMD_HELLO:
        IMSG("\n\n>>> [+] HELLO FROM PTA ATTESTATION SERVICE\n\n");
		
		test_ecdsa_sign_and_verify();
		test_gen_quote();
		
        IMSG("\n\n>>> [-] HELLO FROM PTA ATTESTATION SERVICE\n\n");
		return TEE_SUCCESS;
	
	case ATTESTATION_CMD_GEN_QUOTE:
		if (TEE_PARAM_TYPES(
                    TEE_PARAM_TYPE_MEMREF_INPUT, // claim
                    TEE_PARAM_TYPE_MEMREF_INPUT, // anchor
				    TEE_PARAM_TYPE_MEMREF_INOUT, // quote
				    TEE_PARAM_TYPE_NONE) != types) {
			
			return TEE_ERROR_BAD_PARAMETERS;
		}

		return generate_quote(params[0].memref.buffer, params[0].memref.size,
				params[1].memref.buffer, params[1].memref.size, (ra_quote*) params[2].memref.buffer);

		break;

	default:
		break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}

pseudo_ta_register(.uuid = PTA_ATTESTATION_SERVICE_UUID, .name = PTA_NAME,
		   .flags = PTA_DEFAULT_FLAGS,
		   .create_entry_point = create,
		   .invoke_command_entry_point = invoke_command);
