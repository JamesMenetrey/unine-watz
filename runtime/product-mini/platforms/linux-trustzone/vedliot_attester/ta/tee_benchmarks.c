#include "tee_benchmarks.h"

#include <stdarg.h>
#include <string.h>

static TEE_Time benchmark_storage[HIGHEST_ID];

TEE_Time* benchmark_get_store(uint32_t id) {
    return &benchmark_storage[id];
}

int64_t benchmark_get_value(uint32_t id) {
    return teetime_to_micro(benchmark_storage[id]);
}