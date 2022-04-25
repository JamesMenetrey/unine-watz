#ifndef REMOTE_ATTESTATION_H
#define REMOTE_ATTESTATION_H

#include <attestation.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <tee_isocket.h>
#include <tee_tcpsocket.h>
#include <tee_udpsocket.h>

#define WASI_RA_MSG_BUFFER_SIZE     256
#define WASI_RA_ECDH_KEY_SIZE       256
#define WASI_RA_ECDSA_KEY_SIZE      256
#define WASI_RA_AES_CMAC_KEY_SIZE   128
#define WASI_RA_AES_CMAC_TAG_SIZE   128
#define WASI_RA_AES_GCM_KEY_SIZE    128
#define WASI_RA_AES_GCM_TAG_SIZE    128
#define WASI_RA_AES_GCM_IV_SIZE     32
#define WASI_RA_HASH_SIZE           256
#define WASI_RA_SHARED_MAC_DATA "\x01SMK\x00\x80\x00"
#define WASI_RA_SHARED_SESSION_DATA "\x01SK\x00\x80\x00"

#define WASI_RA_AES_GCM_CIPHERTEXT_OVERHEAD 16

struct sock_handle {
	TEE_iSocketHandle ctx;
	TEE_iSocket *socket;
};

typedef struct
{
    uint8_t ecdh_local_public_key_x_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_x_size;
    uint8_t ecdh_local_public_key_y_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_y_size;
} msg0_t;

typedef struct {
    uint8_t ecdh_verifier_public_key_x_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_verifier_public_key_x_size;
    uint8_t ecdh_verifier_public_key_y_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_verifier_public_key_y_size;
    uint8_t ecdsa_verifier_public_key_x_raw[WASI_RA_ECDSA_KEY_SIZE / 8];
    uint32_t ecdsa_verifier_public_key_x_size;
    uint8_t ecdsa_verifier_public_key_y_raw[WASI_RA_ECDSA_KEY_SIZE / 8];
    uint32_t ecdsa_verifier_public_key_y_size;
    uint8_t ecdh_signature[RA_SIGNATURE_SIZE];
    uint8_t mac[WASI_RA_AES_CMAC_TAG_SIZE / 8];
} msg1_t;

typedef struct {
    uint8_t ecdh_local_public_key_x_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_x_size;
    uint8_t ecdh_local_public_key_y_raw[WASI_RA_ECDH_KEY_SIZE / 8];
    uint32_t ecdh_local_public_key_y_size;
    ra_quote quote;
    uint8_t mac[WASI_RA_AES_CMAC_TAG_SIZE / 8];
} msg2_t;

typedef struct {
    uint8_t iv[WASI_RA_AES_GCM_IV_SIZE];
    uint8_t tag[WASI_RA_AES_GCM_TAG_SIZE / 8];
    uint32_t encrypted_content_size;
    uint8_t encrypted_content[];
} msg3_t;

typedef struct {
    // Keys material
    TEE_ObjectHandle ecdh_local_keypair_handle;
    TEE_Attribute ecdh_remote_public_key_x_attr;
    TEE_Attribute ecdh_remote_public_key_y_attr;
    TEE_ObjectHandle ecdh_shared_key_derivation_key;
    TEE_ObjectHandle ecdh_shared_mac_key;
    TEE_ObjectHandle ecdh_shared_session_key;

    TEE_Attribute ecdsa_verifier_public_key_x_attr;
    TEE_Attribute ecdsa_verifier_public_key_y_attr;
    TEE_ObjectHandle ecdsa_verifier_public_key_handle;

    // Messages
    msg0_t msg0;
    msg1_t msg1;
    msg2_t msg2;
    msg3_t *msg3;
    uint32_t msg3_size;

    // TCP connection
    struct sock_handle socket_handle;
} ra_context;

TEE_Result ra_quote_collect(TEE_TASessionHandle *attestation_session, uint8_t *wasm_bytecode_hash, int wasm_bytecode_hash_size, 
        uint8_t *anchor, int anchor_size, ra_quote *quote);
TEE_Result ra_quote_dispose(TEE_TASessionHandle *attestation_session, ra_quote *quote);
TEE_Result ra_net_handshake(ra_context *ctx, const char* host, uint8_t *ecdsa_service_public_key_x, uint32_t ecdsa_service_public_key_x_size, 
        uint8_t *ecdsa_service_public_key_y, uint32_t ecdsa_service_public_key_y_size, uint8_t *anchor, uint32_t anchor_size);
TEE_Result ra_net_send_quote(ra_context *ctx, ra_quote *quote);
TEE_Result ra_net_receive_data(ra_context *ctx, uint8_t *data, uint32_t *data_size);
TEE_Result ra_net_dispose(ra_context *ctx);

#endif /*REMOTE_ATTESTATION_H*/