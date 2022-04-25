/*
 * Copyright (c) 2016, Linaro Limited
 */

#ifndef __PTA_ATTESTATION_SERVICE
#define __PTA_ATTESTATION_SERVICE

// UUID: a2e8677d-4f37-4cb8-a1b9-df5232404596
#define PTA_ATTESTATION_SERVICE_UUID \
		{ 0xa2e8677d, 0x4f37, 0x4cb8, \
			{ 0xa1, 0xb9, 0xdf, 0x52, 0x32, 0x40, 0x45, 0x96 } }

#define ATTESTATION_CMD_HELLO		0
#define ATTESTATION_CMD_GEN_QUOTE	1

#endif /*__PTA_ATTESTATION_SERVICE*/