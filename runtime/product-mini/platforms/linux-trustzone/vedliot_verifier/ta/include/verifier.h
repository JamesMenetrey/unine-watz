#ifndef VERIFIER_H
#define VERIFIER_H

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <attestation.h>

#define RA_ECDH_KEY_SIZE                        256
#define RA_ECDSA_KEY_SIZE                       256
#define RA_HASH_SIZE                            256
#define RA_AES_CMAC_KEY_SIZE                    128
#define RA_AES_CMAC_TAG_SIZE                    128
#define RA_MSG3_DATA_BUFFER_SIZE                128
#define RA_MSG3_ENCRYPTED_CONTENT_BUFFER_SIZE   192
#define RA_AES_GCM_KEY_SIZE                     128
#define RA_AES_GCM_TAG_SIZE                     128
#define RA_AES_GCM_IV_SIZE                      32
#define RA_SHARED_MAC_DATA "\x01SMK\x00\x80\x00"
#define RA_SHARED_SESSION_DATA "\x01SK\x00\x80\x00"

#define RA_AES_GCM_CIPHERTEXT_OVERHEAD          16

// Taken from the log of the board when the attestation service starts
#define RA_IMX_TEST_PUBLIC_KEY_X {0x48, 0xee, 0xee, 0x81, 0xde, 0x28, 0xdb, 0x3e, 0x5d, 0x5a, 0xfc, 0x7d, 0x4b, 0x7e, 0xa4, 0xb1, 0xac, 0x16, 0xa2, 0xd7, 0xa9, 0x97, 0x8c, 0x1f, 0x84, 0xb4, 0x35, 0x57, 0x30, 0x64, 0x38, 0x47}
#define RA_IMX_TEST_PUBLIC_KEY_Y {0xf7, 0xf2, 0xce, 0x32, 0x43, 0x4e, 0xab, 0x77, 0x93, 0x53, 0x74, 0x8e, 0xee, 0x64, 0xe7, 0x29, 0x76, 0x34, 0x0e, 0x80, 0x5d, 0x87, 0xc2, 0xb6, 0x98, 0x4a, 0xa5, 0xd1, 0x2d, 0x30, 0x3f, 0xaf}

typedef struct
{
    uint8_t ecdh_attester_public_key_x_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_attester_public_key_x_size;
    uint8_t ecdh_attester_public_key_y_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_attester_public_key_y_size;
} msg0_t;

typedef struct
{
    uint8_t ecdh_verifier_public_key_x_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_verifier_public_key_x_size;
    uint8_t ecdh_verifier_public_key_y_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_verifier_public_key_y_size;
    uint8_t ecdsa_verifier_public_key_x_raw[RA_ECDSA_KEY_SIZE / 8];
    uint32_t ecdsa_verifier_public_key_x_size;
    uint8_t ecdsa_verifier_public_key_y_raw[RA_ECDSA_KEY_SIZE / 8];
    uint32_t ecdsa_verifier_public_key_y_size;
    uint8_t ecdh_signature[RA_SIGNATURE_SIZE];
    uint8_t mac[RA_AES_CMAC_TAG_SIZE / 8];
} msg1_t;

typedef struct {
    uint8_t ecdh_local_public_key_x_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_x_size;
    uint8_t ecdh_local_public_key_y_raw[RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_y_size;
    ra_quote quote;
    uint8_t mac[RA_AES_CMAC_TAG_SIZE / 8];
} msg2_t;

typedef struct {
    uint8_t iv[RA_AES_GCM_IV_SIZE];
    uint8_t tag[RA_AES_GCM_TAG_SIZE / 8];
    uint32_t encrypted_content_size;
    uint8_t encrypted_content[];
} msg3_t;

typedef struct
{
    // Secret
    uint8_t *secret;
    uint32_t secret_size;

    // Keys material
    TEE_Attribute ecdsa_attester_public_key_x_attr;
    TEE_Attribute ecdsa_attester_public_key_y_attr;
    TEE_ObjectHandle ecdsa_attester_public_key_handle;

    TEE_Attribute ecdh_attester_public_key_x_attr;
    TEE_Attribute ecdh_attester_public_key_y_attr;
    TEE_ObjectHandle ecdh_attester_public_key_handle;

    TEE_ObjectHandle ecdsa_local_keypair_handle;
    TEE_ObjectHandle ecdh_local_keypair_handle;

    TEE_ObjectHandle ecdh_shared_key_derivation_key;
    TEE_ObjectHandle ecdh_shared_mac_key;
    TEE_ObjectHandle ecdh_shared_session_key;

    bool is_quote_valid;

    // Messages
    msg0_t msg0;
    msg1_t msg1;
    msg2_t msg2;
    msg3_t *msg3;
    uint32_t msg3_size;
} ra_context_verifier;

TEE_Result import_attester_attestation_key(ra_context_verifier *ctx);
TEE_Result generate_ecdsa_keypair(ra_context_verifier *ctx);
TEE_Result handle_msg0(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size);
TEE_Result prepare_msg1(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size, uint32_t *msg1_size);
TEE_Result handle_msg2(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size);
TEE_Result prepare_msg3(ra_context_verifier *ctx, uint8_t *buffer, uint32_t buffer_size, uint8_t *data, uint32_t data_size,
        void *benchmark_buffer, uint32_t benchmark_buffer_size);
TEE_Result dispose_ra_context(ra_context_verifier *ctx);

#endif /*VERIFIER_H*/