#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "genann.h"

/* This example is to illustrate how to use GENANN.
 * It is NOT an example of good machine learning techniques.
 */

#if CFG_TEE_TA_LOG_LEVEL >= 3
#define dprintf printf
#else
#define dprintf(...)
#endif

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

#define IRIS_DATA_SIZE 3 * 1024 * 1024

double *input, *class;
int samples;
const char *class_names[] = {"Iris-setosa", "Iris-versicolor", "Iris-virginica"};

/* The delimitors in each record are commas, while thex are semi-colons between two records. */
const char *record_delimitors[] = {",", ",", ",", ";"};

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

void remote_attestation(char* data, unsigned int data_size) {
    unsigned int res;
    unsigned int quote_handle;
    unsigned int ra_context_handle;

    char anchor[WASI_RA_ANCHOR_SIZE];
    const char host[] = "127.0.0.1";

    BENCHMARK_START(net_handshake);
    res = wasi_ra_net_handshake(host, NULL, 0, NULL, 0, anchor, sizeof(anchor), &ra_context_handle);
    BENCHMARK_STOP(net_handshake);
    if (res != WASI_RA_SUCCESS) {
        printf("Error during wasi_ra_net_handshake: %x\n", res);
        goto out;
    }

    BENCHMARK_START(collect_quote);
    res = wasi_ra_collect_quote(anchor, sizeof(anchor), &quote_handle);
    BENCHMARK_STOP(collect_quote);
    if (res != WASI_RA_SUCCESS) {
        printf("Error during wasi_ra_collect_quote: %x\n", res);
        goto out;
    }

    printf("%lld,", timespec_to_micro(collect_quote));
    printf("%lld,", timespec_to_micro(net_handshake));

    BENCHMARK_START(send_quote);
    res = wasi_ra_net_send_quote(ra_context_handle, quote_handle);
    BENCHMARK_STOP(send_quote);
    if (res != WASI_RA_SUCCESS) {
        printf("Error during wasi_ra_net_send_quote: %x\n", res);
        goto out;
    }

    printf("%lld,", timespec_to_micro(send_quote));

    BENCHMARK_START(receive_data);
    res = wasi_ra_net_receive_data(ra_context_handle, data, &data_size);
    BENCHMARK_STOP(receive_data);
    if (res != WASI_RA_SUCCESS) {
        printf("0,0,0,0,0,Error during wasi_ra_net_receive_data: %x\n", res);
        goto out;
    }

    printf("%lld,", timespec_to_micro(receive_data));
    
out:

    if (wasi_ra_net_dispose(ra_context_handle) != WASI_RA_SUCCESS) {
        printf("Error during wasi_ra_net_dispose: %x\n", res);
    }

    if (wasi_ra_net_dispose_quote(quote_handle) != WASI_RA_SUCCESS) {
        printf("Error during wasi_ra_net_dispose_quote: %x\n", res);
    }

    if (res != WASI_RA_SUCCESS) {
        exit(1);
    }
}

void import_data_ree(char* raw_data, unsigned int data_size, int dataset_size) {
    BENCHMARK_START(importfile);

    char filename[30];
	snprintf(filename, 30, "genann/iris-%d.data", dataset_size);

	FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Could not open file: %s\n", filename);
        exit(1);
    }

    fgets((char*) raw_data, data_size, f);
    fclose(f);

    BENCHMARK_STOP(importfile);
    printf("%lld,", timespec_to_micro(importfile));
}

void import_data_tee(char* raw_data, unsigned int data_size) {
    /* Remotely attest this app and get the dataset from there. */
    remote_attestation(raw_data, data_size);
}

void load_data(char* raw_data, unsigned int data_size) {
    BENCHMARK_START(loaddata);

    /* Count the number of records. */
    samples = 0;
    char *cursor = raw_data;
    while((cursor = strchr(cursor, ';')) != NULL) {
        samples++;
        cursor++;
    }

    dprintf("Loading %d data points\n", samples);

    /* Allocate memory for input and output data. */
    input = malloc(sizeof(double) * samples * 4);
    class = malloc(sizeof(double) * samples * 3);

    /* Read the file into our arrays. */
    char *split = strtok(raw_data, ",");

    int i, j;
    for (i = 0; i < samples; ++i) {
        double *p = input + i * 4;
        double *c = class + i * 3;
        c[0] = c[1] = c[2] = 0.0;
 
        for (j = 0; j < 4; ++j) {
            p[j] = atof(split);
            split = strtok(0, record_delimitors[j]);
        }

        if (strcmp(split, class_names[0]) == 0) {c[0] = 1.0;}
        else if (strcmp(split, class_names[1]) == 0) {c[1] = 1.0;}
        else if (strcmp(split, class_names[2]) == 0) {c[2] = 1.0;}
        else {
            printf("[%d] Unknown class %s.\n", i, split);
            exit(1);
        }

        split = strtok(0, ",");

        /* printf("Data point %d is %f %f %f %f  ->   %f %f %f\n", i, p[0], p[1], p[2], p[3], c[0], c[1], c[2]); */
    }

    BENCHMARK_STOP(loaddata);
    printf("%lld,", timespec_to_micro(loaddata));
}

int main(int argc, char *argv[])
{
    dprintf("GENANN example 4.\n");
    dprintf("Train an ANN on the IRIS dataset using backpropagation.\n");

    srand(0);

    char *iris_data = malloc(IRIS_DATA_SIZE);

    /* Import the data into the buffer. */
#ifdef TEE
    import_data_tee(iris_data, IRIS_DATA_SIZE);
#endif
#ifdef REE
    if (argc != 2) {
        printf("Error: the dataset size is expected as an argument.\n");
        exit(1);
    }

    import_data_ree(iris_data, IRIS_DATA_SIZE, atoi(argv[1]));
#endif
    
    /* Load the data from the buffer. */
    load_data(iris_data, IRIS_DATA_SIZE);

    /* 4 inputs.
     * 1 hidden layer(s) of 4 neurons.
     * 3 outputs (1 per class)
     */
    genann *ann = genann_init(4, 1, 4, 3);

    int i, j;
    int loops = 200;

    /* Train the network with backpropagation. */
    BENCHMARK_START(training);
    dprintf("Training for %d loops over data.\n", loops);
    for (i = 0; i < loops; ++i) {
        for (j = 0; j < samples; ++j) {
            genann_train(ann, input + j*4, class + j*3, .01);
        }
        /* printf("%1.2f ", xor_score(ann)); */
    }
    BENCHMARK_STOP(training);
    printf("%lld,", timespec_to_micro(training));

    BENCHMARK_START(predict);
    int correct = 0;
    for (j = 0; j < samples; ++j) {
        const double *guess = genann_run(ann, input + j*4);
        if (class[j*3+0] == 1.0) {if (guess[0] > guess[1] && guess[0] > guess[2]) ++correct;}
        else if (class[j*3+1] == 1.0) {if (guess[1] > guess[0] && guess[1] > guess[2]) ++correct;}
        else if (class[j*3+2] == 1.0) {if (guess[2] > guess[0] && guess[2] > guess[1]) ++correct;}
        else {printf("Logic error.\n"); exit(1);}
    }
    BENCHMARK_STOP(predict);
    printf("%lld,1,\n", timespec_to_micro(predict));

    dprintf("%d/%d correct (%0.1f%%).\n", correct, samples, (double)correct / samples * 100.0);

    genann_free(ann);
    free(input);
    free(class);

out:

    //free(iris_data);

    return 0;
}
