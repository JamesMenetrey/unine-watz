#ifndef __VEDLIOT_CRYPTO_H
#define __VEDLIOT_CRYPTO_H

#include <tee_api_types.h>

TEE_Result gen_ecdsa_key_with_seed(uint8_t *seed, int seed_len, struct ecc_keypair *key,
        size_t key_size);

TEE_Result ecc_export_public_in_buffer(struct ecc_keypair *key, uint32_t algo, unsigned char *out, unsigned long *outlen);

#endif /*__VEDLIOT_CRYPTO_H*/