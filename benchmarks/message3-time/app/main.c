/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include "data_size.h"

// RA imports
__attribute__((import_name("wasi_ra_collect_quote"))) int wasi_ra_collect_quote(void *anchor, int anchor_size, unsigned int *quote_handle);
__attribute__((import_name("wasi_ra_net_dispose_quote"))) int wasi_ra_net_dispose_quote(unsigned int quote_handle);
__attribute__((import_name("wasi_ra_net_handshake"))) int wasi_ra_net_handshake(const char *host, void *ecdsa_service_public_key_x,
        unsigned int ecdsa_service_public_key_x_size, void *ecdsa_service_public_key_y, unsigned int ecdsa_service_public_key_y_size,
        void *anchor, unsigned int anchor_size, unsigned int *ra_context_handle_out);
__attribute__((import_name("wasi_ra_net_send_quote"))) int wasi_ra_net_send_quote(unsigned int ra_context_handle, unsigned int quote_handle);
__attribute__((import_name("wasi_ra_net_receive_data"))) int wasi_ra_net_receive_data(unsigned int ra_context_handle, void *data, unsigned int *data_size);
__attribute__((import_name("wasi_ra_net_dispose"))) int wasi_ra_net_dispose(unsigned int ra_context_handle);

#define WASI_RA_SUCCESS 0x0
#define WASI_RA_ANCHOR_SIZE 32

int main(int argc, char **argv)
{
    char *buf;
    unsigned int res = 0xffffffff;
    unsigned int quote_handle = 0xffffffff;
    unsigned int ra_context_handle = 0xffffffff;

    const char host[] = "127.0.0.1";
    char anchor[WASI_RA_ANCHOR_SIZE];

    res = wasi_ra_net_handshake(host, NULL, 0, NULL, 0, anchor, sizeof(anchor), &ra_context_handle);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_net_handshake: %x\n", res);
        goto out;
    }

    res = wasi_ra_collect_quote(anchor, sizeof(anchor), &quote_handle);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_collect_quote: %x\n", res);
        goto out;
    }

    res = wasi_ra_net_send_quote(ra_context_handle, quote_handle);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_net_send_quote: %x\n", res);
        goto out;
    }

    unsigned int data_size = DATA_SIZE;
    unsigned char *data = malloc(data_size);

    res = wasi_ra_net_receive_data(ra_context_handle, data, &data_size);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_net_receive_data: %x\n", res);
        goto out;
    }

    if (data[0] != 'A') {
        printf("0,The data does not match the one sent by the verifier.\n");
        goto out;
    }

    printf("1,");

out:

    res = wasi_ra_net_dispose(ra_context_handle);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_net_dispose: %x\n", res);
    }

    res = wasi_ra_net_dispose_quote(quote_handle);
    if (res != WASI_RA_SUCCESS) {
        printf("0,Error during wasi_ra_net_dispose_quote: %x\n", res);
    }
    
    free(data);

    return 0;
}
