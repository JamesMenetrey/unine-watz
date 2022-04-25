/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>

__attribute__((import_name("wasi_ra_collect_quote"))) int wasi_ra_collect_quote(void *anchor, int anchor_size, unsigned int *quote_handle);
__attribute__((import_name("wasi_ra_net_dispose_quote"))) int wasi_ra_net_dispose_quote(unsigned int quote_handle);
__attribute__((import_name("wasi_ra_net_handshake"))) int wasi_ra_net_handshake(const char *host, void *ecdsa_service_public_key_x,
        unsigned int ecdsa_service_public_key_x_size, void *ecdsa_service_public_key_y, unsigned int ecdsa_service_public_key_y_size,
        void *anchor, unsigned int anchor_size, unsigned int *ra_context_handle_out);
__attribute__((import_name("wasi_ra_net_send_quote"))) int wasi_ra_net_send_quote(unsigned int ra_context_handle, unsigned int quote_handle);
__attribute__((import_name("wasi_ra_net_receive_data"))) int wasi_ra_net_receive_data(unsigned int ra_context_handle, void *data, unsigned int *data_size);
__attribute__((import_name("wasi_ra_net_dispose"))) int wasi_ra_net_dispose(unsigned int ra_context_handle);

#define WASI_RA_ANCHOR_SIZE 32

int main(int argc, char **argv)
{
    char *buf;
    unsigned int res = 0xffffffff;
    unsigned int quote_handle = 0xffffffff;
    unsigned int ra_context_handle = 0xffffffff;
    char anchor[WASI_RA_ANCHOR_SIZE];

    if (argc > 1) {
        printf("Hello %s!\n", argv[1]);
    } else {
        printf("Hello world!\n");
    }

    buf = malloc(1024);
    if (!buf) {
        printf("malloc buf failed\n");
        return -1;
    }

    printf("buf ptr: %p\n", buf);

    snprintf(buf, 1024, "%s", "1234\n");
    printf("buf: %s", buf);

    free(buf);

    const char host[] = "127.0.0.1";
    res = wasi_ra_net_handshake(host, NULL, 0, NULL, 0, anchor, sizeof(anchor), &ra_context_handle);
    printf("wasi_ra_net_handshake called; ra_context handle: %u; res: %u\n", ra_context_handle, res);

    res = wasi_ra_collect_quote(anchor, sizeof(anchor), &quote_handle);
    printf("wasi_ra_collect_quote called; quote handle: %u; res: %u\n", quote_handle, res);

    res = wasi_ra_net_send_quote(ra_context_handle, quote_handle);
    printf("wasi_ra_net_send_quote returned %u\n", res);

    unsigned int data_size = 1024;
    unsigned char *data = malloc(data_size);
    res = wasi_ra_net_receive_data(ra_context_handle, data, &data_size);
    printf("wasi_ra_net_receive_data returned %u; data size: %u; data (limited to 40): %.40s\n", res, data_size, data);

    res = wasi_ra_net_dispose(ra_context_handle);
    printf("wasi_ra_net_dispose returned %u\n", res);

    res = wasi_ra_net_dispose_quote(quote_handle);
    printf("wasi_ra_net_dispose_quote returned %u\n", res);

    free(data);

    return 0;
}
