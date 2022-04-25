// Standard C library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// BSD headers
#include <err.h>

// GlobalPlatform Client API
#include <tee_client_api.h>

// GlobalPlatfrom TA
#include <mwe_ta.h>

// Inspired from sys/time.h
# define timespec_diff(a, b, result)                  \
  do {                                                \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;  \
    if ((result)->tv_nsec < 0) {                      \
      --(result)->tv_sec;                             \
      (result)->tv_nsec += 1000000000;                \
    }                                                 \
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

typedef struct {
    TEEC_Context context;
    TEEC_Session session;
} tee_context;

static void initialize_context(tee_context *tee) {
    TEEC_Result res;

    res = TEEC_InitializeContext(NULL, &tee->context);
    if (res != TEEC_SUCCESS)
        errx(EXIT_FAILURE, "TEEC_InitializeContext failed with code %#x", res);
}

static void open_session(tee_context *tee) {
    TEEC_Result res;
    TEEC_UUID uuid = TA_MWE_UUID;
    uint32_t err_origin;

    res = TEEC_OpenSession(&tee->context, &tee->session, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS)
        errx(EXIT_FAILURE, "TEEC_OpenSession failed with code %#x origin %#x", res, err_origin);
}

static void benchmark_roundtrip(tee_context *tee, uint32_t iterations) {
    TEEC_Result res;
    TEEC_Operation op;
    uint32_t err_origin;
    struct timespec before, ta, after, enter, leave;

    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &before);
        res = TEEC_InvokeCommand(&tee->session, TA_COMMAND_ROUNDTRIP, &op, &err_origin);
        clock_gettime(CLOCK_MONOTONIC, &after);

        if (res != TEEC_SUCCESS) {
            printf("TEEC_InvokeCommand failed with code %#x origin %#x", res, err_origin);
        }
        
        // Parse the time from the TA
        ta.tv_sec = op.params[0].value.a;
        ta.tv_nsec = op.params[0].value.b;

        // Compute the difference of time
        timespec_diff(&ta, &before, &enter);
        timespec_diff(&after, &ta, &leave);

        printf("%ld, %ld\n", timespec_to_micro(enter), timespec_to_micro(leave));
    }
}

static void benchmark_tatime(tee_context *tee, uint32_t iterations) {
    TEEC_Result res;
    TEEC_Operation op;
    uint32_t err_origin;
    struct timespec time;

    memset(&op, 0, sizeof(op));

    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

    for (int i = 0; i < iterations; i++) {
        res = TEEC_InvokeCommand(&tee->session, TA_COMMAND_GETREETIME, &op, &err_origin);
        
        if (res != TEEC_SUCCESS) {
            printf("TEEC_InvokeCommand failed with code %#x origin %#x", res, err_origin);
        }

        time.tv_sec = op.params[0].value.a;
        time.tv_nsec = op.params[0].value.b;

        printf("%ld\n", timespec_to_micro(time));
    }
}

static void benchmark_catime(tee_context *tee, uint32_t iterations) {

    for (int i = 0; i < iterations; i++) {
        BENCHMARK_START(nanos);
        BENCHMARK_STOP(nanos);

        printf("%ld\n", timespec_to_micro(nanos));
    }
}

static void close_session(tee_context *tee) {
    TEEC_CloseSession(&tee->session);
}

static void finalize_context(tee_context *tee) {
    TEEC_FinalizeContext(&tee->context);
}

int main(int argc, char *argv[]) {
    tee_context tee;
    char *benchmark_name;
    uint32_t number_of_iterations;

    if (argc != 3) {
        printf("usage: benchmark_name number_of_iterations\n");
        exit(1);
    }

    benchmark_name = argv[1];
    number_of_iterations = atoi(argv[2]);

    initialize_context(&tee);
    open_session(&tee);

    if (strcmp(benchmark_name, "roundtrip") == 0) {
        benchmark_roundtrip(&tee, number_of_iterations);
    }
    else if (strcmp(benchmark_name, "tatime") == 0) {
        benchmark_tatime(&tee, number_of_iterations);
    }
    else if (strcmp(benchmark_name, "catime") == 0) {
        benchmark_catime(&tee, number_of_iterations);
    }
    else {
        printf("The benchmark '%s' is unknown.\n", benchmark_name);
    }

    close_session(&tee);
    finalize_context(&tee);

    return EXIT_SUCCESS;
}
