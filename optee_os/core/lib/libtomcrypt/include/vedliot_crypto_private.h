#ifndef __VEDLIOT_CRYPTO_PRIVATE_H
#define __VEDLIOT_CRYPTO_PRIVATE_H

#include <tee_api_types.h>
#include <tomcrypt.h>

TEE_Result _ltc_ecc_generate_keypair_with_prng(struct ecc_keypair *key, size_t key_size, prng_state *prng);

#endif /*__VEDLIOT_CRYPTO_PRIVATE_H*/
