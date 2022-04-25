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

#define BENCHMARK_BUFFER_SIZE 1024

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

static void close_session(tee_context *tee) {
    TEEC_CloseSession(&tee->session);
}

static void finalize_context(tee_context *tee) {
    TEEC_FinalizeContext(&tee->context);
}

static void run_speedtest1(tee_context *tee) {
    TEEC_Result res;
    TEEC_Operation op;
    uint32_t err_origin;

    memset(&op, 0, sizeof(op));

    char* benchmark_buffer = malloc(BENCHMARK_BUFFER_SIZE);

    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
    op.params[0].tmpref.buffer = benchmark_buffer;
	op.params[0].tmpref.size = BENCHMARK_BUFFER_SIZE;
    
    res = TEEC_InvokeCommand(&tee->session, TA_COMMAND_RUN_SPEEDTEST1, &op, &err_origin);
    
    if (res != TEEC_SUCCESS) {
        printf("TEEC_InvokeCommand failed with code %#x origin %#x", res, err_origin);
    }

    printf("%s\n", benchmark_buffer);

    free(benchmark_buffer);
}

int main(int argc, char *argv[]) {
    tee_context tee;

    initialize_context(&tee);
    open_session(&tee);

    run_speedtest1(&tee);

    close_session(&tee);
    finalize_context(&tee);

    return EXIT_SUCCESS;
}
