#include <config.h>
#include <crypto/crypto_impl.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include <utee_defines.h>
#include <tomcrypt.h>

#include "acipher_helpers.h"
#include "vedliot_crypto.h"
#include "vedliot_crypto_private.h"

TEE_Result gen_ecdsa_key_with_seed(uint8_t *seed, int seed_len, struct ecc_keypair *key, size_t key_size)
{
    prng_state prng;
	TEE_Result res;
	int ltc_res;

	if ((ltc_res = register_all_prngs()) != CRYPT_OK)
	{
		EMSG("The PRNGs cannot be registered. Error: %s\n", error_to_string(ltc_res));
		return TEE_ERROR_GENERIC;
	}

	if ((ltc_res = fortuna_start(&prng)) != CRYPT_OK) {
		EMSG("The Fortuna PRNG cannot be started. Error: %s\n", error_to_string(ltc_res));
		return TEE_ERROR_GENERIC;
	}

	if ((ltc_res = fortuna_add_entropy(seed, seed_len, &prng)) != CRYPT_OK) {
		EMSG("The seed cannot be added as a source of entropy. Error: %s\n", error_to_string(ltc_res));
		return TEE_ERROR_GENERIC;
	}

	if ((ltc_res = fortuna_ready(&prng)) != CRYPT_OK) {
		EMSG("The initialization of Fortuna PRNG cannot be finished. Error: %s\n", error_to_string(ltc_res));
		return TEE_ERROR_GENERIC;
	}

	if ((res = _ltc_ecc_generate_keypair_with_prng(key, key_size, &prng)) != TEE_SUCCESS) {
		EMSG("The ECC keypair cannot be generated. Error: %x", res);
		return res;
	}

	return TEE_SUCCESS;
}

TEE_Result ecc_export_public_in_buffer(struct ecc_keypair *key, uint32_t algo, unsigned char *out, unsigned long *outlen) {
	TEE_Result res = TEE_SUCCESS;
	int ltc_res;
	ecc_key ltc_key = { };
	size_t key_size_bytes;

	// Retrieve the ltc key from the abstraction layer
	ltc_res = ecc_populate_ltc_private_key(&ltc_key, key, algo, &key_size_bytes);
	if (ltc_res != CRYPT_OK) {
		EMSG("The LTC ECC keypair cannot be retrieved. Error: %s", error_to_string(ltc_res));
		res = TEE_ERROR_GENERIC;
		goto out;
	}

	// Export the public key
	if((ltc_res = ecc_get_key(out, outlen, PK_PUBLIC, (const ecc_key*) &ltc_key)) != CRYPT_OK)
	{
		EMSG("The public ECC key cannot be exported. Error: %s\n", error_to_string(ltc_res));
		res = TEE_ERROR_GENERIC;
		goto out;
	}

out:
	// Free the ltc key
	ecc_free(&ltc_key);
	return res;
}