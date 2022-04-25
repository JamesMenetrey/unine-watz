/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Inspired from sys/time.h
#define timespec_diff(a, b, result)                     \
    do {                                                \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;   \
        (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;\
        if ((result)->tv_nsec < 0) {                    \
            --(result)->tv_sec;                         \
        (result)->tv_nsec += 1000000000;                \
        }                                               \
    } while (0)

#define timespec_to_micro(t) \
    t.tv_sec * 1000 * 1000 + t.tv_nsec / 1000

#define BENCHMARK_START(X)                          \
    struct timespec start_##X, end_##X, X;   \
    clock_gettime(CLOCK_MONOTONIC, &start_##X)      \

#define BENCHMARK_STOP(X)                                   \
    do {                                                    \
        clock_gettime(CLOCK_MONOTONIC, &end_##X);           \
        timespec_diff(&end_##X, &start_##X, &X);            \
    } while(0)

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: number_of_iterations\n");
        return 1;
    }

    unsigned int number_of_iterations = atoi(argv[1]);

    for (int i = 0; i < number_of_iterations; i++) {
        BENCHMARK_START(nanos);
        BENCHMARK_STOP(nanos);

      printf("%lld\n", timespec_to_micro(nanos));
    }

    return 0;
}
