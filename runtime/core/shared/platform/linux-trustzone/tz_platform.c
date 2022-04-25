/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define FIXED_BUFFER_SIZE (1<<9)

int bh_platform_init()
{
    return 0;
}

void
bh_platform_destroy()
{
}

void *
os_malloc(unsigned size)
{
    return TEE_Malloc(size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
}

void *
os_realloc(void *ptr, unsigned size)
{
    return TEE_Realloc(ptr, size);
}

void
os_free(void *ptr)
{
    TEE_Free(ptr);
}

int os_printf(const char *message, ...)
{
    /*
     * Defined static to reduce the pressure on the stack allocation.
     * The TA may panic otherwise. Note this implementation may cause
     * concurrent accesses issues when the application is multithreaded,
     * which is not the case in the TA.
     */
    static char msg[FIXED_BUFFER_SIZE] = { '\0' };
    va_list ap;
    va_start(ap, message);
    vsnprintf(msg, FIXED_BUFFER_SIZE, message, ap);
    va_end(ap);

    IMSG(msg);

    return 0;
}

int os_vprintf(const char * format, va_list arg)
{
    /*
     * Defined static to reduce the pressure on the stack allocation.
     * The TA may panic otherwise. Note this implementation may cause
     * concurrent accesses issues when the application is multithreaded,
     * which is not the case in the TA.
     */
    static char msg[FIXED_BUFFER_SIZE] = { '\0' };
    vsnprintf(msg, FIXED_BUFFER_SIZE, format, arg);
    
    EMSG(msg);

    return 0;
}

char *strcpy(char *dest, const char *src)
{
    const unsigned char *s = src;
    unsigned char *d = dest;

    while ((*d++ = *s++));
    return dest;
}

void* os_mmap(void *hint, size_t size, int prot, int flags)
{
    uint64 aligned_size, page_size;
    void* ret = NULL;

    page_size = PAGE_SIZE;
    aligned_size = (size + page_size - 1) & ~(page_size - 1);

    if (aligned_size >= UINT32_MAX)
        return NULL;

    ret = tee_map_zi(aligned_size, 0);
    if (ret == NULL) {
        os_printf("os_mmap(size=%lu, aligned size=%lu, prot=0x%x) failed.",
                  size, aligned_size, prot);
        return NULL;
    }
    else {
        DMSG("os_mmap(addr=%p, size=%lu, aligned size=%lu, prot=0x%x) memory allocated.",
                  ret, size, aligned_size, prot);
    }

    if (os_mprotect(ret, aligned_size, prot) != 0) {
        os_printf("os_mmap(size=%lu, prot=0x%x) failed to set protect.",
                  size, prot);
        tee_unmap(ret, aligned_size);
        return NULL;
    }
    else {
        DMSG("os_mmap(addr=%p, size=%lu, aligned size=%lu, prot=0x%x) protection set.",
                  ret, size, aligned_size, prot);
    }

    return ret;
}

void os_munmap(void *addr, size_t size)
{
    uint64 aligned_size, page_size;
    TEE_Result res;

    page_size = PAGE_SIZE;
    aligned_size = (size + page_size - 1) & ~(page_size - 1);
    
    res = tee_unmap(addr, aligned_size);
    if (res != TEE_SUCCESS) {
        os_printf("os_munmap(addr=%p, size=%lu, aligned size=%lu) error while unmapping the memory: %u.",
                  addr, size, aligned_size, res);
    } else {
        DMSG("os_munmap(addr=%p, size=%lu, aligned size=%lu) OK.",
                  addr, size, aligned_size);
    }
}

int os_mprotect(void *addr, size_t size, int prot)
{
    int mprot = 0;
    TEE_Result res;
    
    if (prot & MMAP_PROT_READ)
        mprot |= TEE_MATTR_UR | TEE_MATTR_PR;
    if (prot & MMAP_PROT_WRITE)
        mprot |= TEE_MATTR_UW | TEE_MATTR_PW;
    if (prot & MMAP_PROT_EXEC)
        mprot |= TEE_MATTR_UX | TEE_MATTR_PX;

    res = tee_mprotect(addr, size, mprot);
    if(res != TEE_SUCCESS) {
        os_printf("os_mprotect(addr=%p, size=%lu, prot=%u) failed: %u.",
                addr, size, prot, res);
    } else {
        DMSG("os_mprotect(addr=%p, size=%lu, prot=%u) OK.",
                addr, size, prot);
    }

    return (res == TEE_SUCCESS? 0:-1);
}

void
os_dcache_flush(void)
{
}

float strtof(const char* str, char** endptr)
{
    EMSG("strtof is not supported.");
    return 0;
}

long int strtol(const char *str, char **endptr, int base)
{
    return strtoul(str, endptr, base);
}

double strtod(const char* str, char** endptr)
{
    EMSG("strtod is not supported.");
    return 0;
}

unsigned long long int strtoull(const char* str, char** endptr, int base)
{
    EMSG("strtoull is not supported.");
    return 0;
}

float sqrtf(float arg)
{
    EMSG("sqrtf is not supported.");
    return 0;
}