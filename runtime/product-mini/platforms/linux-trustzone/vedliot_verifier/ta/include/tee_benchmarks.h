#ifndef TEE_BENCHMARKS_H
#define TEE_BENCHMARKS_H

#include <inttypes.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

// Inspired from utee_defines.h
#define TEE_TIME_NANOS_BASE    1000 * 1000 * 1000
#define TEE_TIME_NANOS_SUB(t1, t2, dst) do {                            \
        (dst).seconds = (t1).seconds - (t2).seconds;                    \
        if ((t1).nanos < (t2).nanos) {                                  \
            (dst).seconds--;                                            \
            (dst).nanos = (t1).nanos + TEE_TIME_NANOS_BASE - (t2).nanos;\
        } else {                                                        \
            (dst).nanos = (t1).nanos - (t2).nanos;                      \
        }                                                               \
    } while (0)

#define teetime_to_micro(t) \
    (long int)t.seconds * 1000 * 1000 + (long int)t.nanos / 1000
#define teetime_to_micro_ref(t) \
    (long int)t->seconds * 1000 * 1000 + (long int)t->nanos / 1000

#define PROFILING_MESSAGES_MESSAGE0_KEYGEN1_START 1
#define PROFILING_MESSAGES_MESSAGE0_MEM_START 2
#define PROFILING_MESSAGES_MESSAGE0_KEYGEN2_START 3
#define PROFILING_MESSAGES_MESSAGE0_KEYGEN2_END 4
#define PROFILING_MESSAGES_MESSAGE1_MEM1_START 5
#define PROFILING_MESSAGES_MESSAGE1_ASYM_CRYPTO_START 6
#define PROFILING_MESSAGES_MESSAGE1_SYM_CRYPTO_START 7
#define PROFILING_MESSAGES_MESSAGE1_MEM2_START 8
#define PROFILING_MESSAGES_MESSAGE1_MEM2_END 9
#define PROFILING_MESSAGES_MESSAGE2_MEM1_START 10
#define PROFILING_MESSAGES_MESSAGE2_SYM_CRYPTO_START 11
#define PROFILING_MESSAGES_MESSAGE2_MEM2_START 12
#define PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_START 13
#define PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_END 14

#define PROFILING_MESSAGE3_MALLOC1_START 0
#define PROFILING_MESSAGE3_MALLOC1_END 1
#define PROFILING_MESSAGE3_MALLOC2_START 2
#define PROFILING_MESSAGE3_MALLOC2_END 3
#define PROFILING_MESSAGE3_ENCRYPT_START 4
#define PROFILING_MESSAGE3_ENCRYPT_END 5

#define HIGHEST_ID  PROFILING_MESSAGES_MESSAGE2_ASYM_CRYPTO_END + 1

TEE_Time* benchmark_get_store(uint32_t id);
int64_t benchmark_get_value(uint32_t id);

#endif