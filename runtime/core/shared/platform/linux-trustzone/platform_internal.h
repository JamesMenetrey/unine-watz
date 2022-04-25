/*
 * Copyright (C) 2021 University of Neuchatel.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "tz_file.h"
#include "tz_pthread.h"
#include "tz_signal.h"
#include "tz_socket.h"
#include "tz_string.h"
#include "tz_time.h"

/* Symbols defined and imported from the kernel of OP-TEE (core). */
#define UINT32_C(c)	c ## U
#define BIT32(nr)		(UINT32_C(1) << (nr))
#define BIT(nr)			BIT32(nr)
#define TEE_MATTR_VALID_BLOCK		BIT(0)
#define TEE_MATTR_TABLE			BIT(3)
#define TEE_MATTR_PR			BIT(4)
#define TEE_MATTR_PW			BIT(5)
#define TEE_MATTR_PX			BIT(6)
#define TEE_MATTR_PRW			(TEE_MATTR_PR | TEE_MATTR_PW)
#define TEE_MATTR_PRX			(TEE_MATTR_PR | TEE_MATTR_PX)
#define TEE_MATTR_PRWX			(TEE_MATTR_PRW | TEE_MATTR_PX)
#define TEE_MATTR_UR			BIT(7)
#define TEE_MATTR_UW			BIT(8)
#define TEE_MATTR_UX			BIT(9)
#define TEE_MATTR_URW			(TEE_MATTR_UR | TEE_MATTR_UW)
#define TEE_MATTR_URX			(TEE_MATTR_UR | TEE_MATTR_UX)
#define TEE_MATTR_URWX			(TEE_MATTR_URW | TEE_MATTR_UX)
#define PAGE_SIZE (4 * 1024)

/* math functions which are not provided by OP-TEE */
double sqrt(double x);
double floor(double x);
double ceil(double x);
double fmin(double x, double y);
double fmax(double x, double y);
double rint(double x);
double fabs(double x);
double trunc(double x);
float floorf(float x);
float ceilf(float x);
float fminf(float x, float y);
float fmaxf(float x, float y);
float rintf(float x);
float truncf(float x);
int signbit(double x);
int isnan(double x);

#ifdef __cplusplus
extern "C" {
#endif

typedef void* korp_thread;
typedef void* korp_tid;
typedef void* korp_mutex;
typedef void* korp_cond;

/* Function prototypes */

#ifdef __cplusplus
}
#endif

#endif  /* end of _PLATFORM_INTERNAL_H */
