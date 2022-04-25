// Standard C library headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// BSD headers
#include <err.h>

// GlobalPlatform Client API
#include <tee_client_api.h>

#include "vedliot_verifier_ta.h"

#define MAX 256
#define PORT 8080
#define SA struct sockaddr
#define SECRET_DATA "Patience is a key element of success."

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

#define WASI_RA_AES_GCM_IV_SIZE    		32
#define RA_AES_GCM_TAG_SIZE     		128
#define RA_AES_GCM_CIPHERTEXT_OVERHEAD	16
typedef struct {
    uint8_t iv[WASI_RA_AES_GCM_IV_SIZE];
    uint8_t tag[RA_AES_GCM_TAG_SIZE / 8];
    uint32_t encrypted_content_size;
    uint8_t encrypted_content[];
} msg3_t;

/* TEE resources */
typedef struct _tee_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
} tee_ctx;

static uint8_t *secret = (uint8_t*)SECRET_DATA;
static uint32_t secret_size = sizeof(SECRET_DATA);

static void prepare_tee_session(tee_ctx* ctx)
{
	TEEC_UUID uuid = VEDLIOT_VERIFIER_TA_UUID;
	uint32_t origin;
	TEEC_Result res;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx->ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/* Open a session with the TA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, origin);
}

static void bootstrap_secret(tee_ctx* ctx, uint8_t *secret, uint32_t secret_size) {
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = secret;
	op.params[0].tmpref.size = secret_size;

	res = TEEC_InvokeCommand(&ctx->sess, VERIFIER_COMMAND_BOOTSTRAP_SECRET, &op, &origin);
	if (res != TEEC_SUCCESS)
    {
		errx(1, "TEEC_InvokeCommand(VERIFIER_COMMAND_BOOTSTRAP_SECRET) failed 0x%x origin 0x%x", res, origin);
        exit(1);
    }
}

static void receive_msg0(tee_ctx* ctx, uint8_t *buffer, size_t buffer_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = buffer;
	op.params[0].tmpref.size = buffer_size;

	res = TEEC_InvokeCommand(&ctx->sess, VERIFIER_COMMAND_HANDLE_MSG_0, &op, &origin);
	if (res != TEEC_SUCCESS)
    {
		errx(1, "TEEC_InvokeCommand(VERIFIER_COMMAND_HANDLE_MSG_0) failed 0x%x origin 0x%x", res, origin);
        exit(1);
    }
}

static uint32_t prepare_msg1(tee_ctx* ctx, uint8_t *buffer, size_t buffer_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = buffer;
	op.params[0].tmpref.size = buffer_size;

	res = TEEC_InvokeCommand(&ctx->sess, VERIFIER_COMMAND_PREPARE_MSG_1, &op, &origin);
	if (res != TEEC_SUCCESS)
    {
		errx(1, "TEEC_InvokeCommand(VERIFIER_COMMAND_PREPARE_MSG_1) failed 0x%x origin 0x%x", res, origin);
        exit(1);
    }

	return op.params[1].value.a;
}

static void receive_msg2(tee_ctx* ctx, uint8_t *buffer, size_t buffer_size, uint8_t *benchmark_buffer, size_t benchmark_buffer_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
	op.params[0].tmpref.buffer = buffer;
	op.params[0].tmpref.size = buffer_size;
	op.params[2].tmpref.buffer = benchmark_buffer;
	op.params[2].tmpref.size = benchmark_buffer_size;

	res = TEEC_InvokeCommand(&ctx->sess, VERIFIER_COMMAND_HANDLE_MSG_2, &op, &origin);
	if (res != TEEC_SUCCESS)
    {
		errx(1, "TEEC_InvokeCommand(VERIFIER_COMMAND_HANDLE_MSG_2) failed 0x%x origin 0x%x", res, origin);
        exit(1);
    }

	if (op.params[1].value.a) {
		dprintf("The quote is valid!\n");
	} else {
		printf("The quote is NOT valid.\n");
	}
}

static void prepare_msg3(tee_ctx* ctx, uint8_t *buffer, size_t buffer_size, uint8_t *benchmark_buffer, size_t benchmark_buffer_size)
{
	TEEC_Operation op;
	uint32_t origin;
	TEEC_Result res;

	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = buffer;
	op.params[0].tmpref.size = buffer_size;
	op.params[1].tmpref.buffer = benchmark_buffer;
	op.params[1].tmpref.size = benchmark_buffer_size;

	res = TEEC_InvokeCommand(&ctx->sess, VERIFIER_COMMAND_PREPARE_MSG_3, &op, &origin);
	if (res != TEEC_SUCCESS)
    {
		errx(1, "TEEC_InvokeCommand(VERIFIER_COMMAND_PREPARE_MSG_3) failed 0x%x origin 0x%x", res, origin);
        exit(1);
    }
}

static void handle_connection(int sockfd, tee_ctx* ctx)
{
	uint32_t buff_size = secret_size + sizeof(msg3_t) + RA_AES_GCM_CIPHERTEXT_OVERHEAD;
    uint8_t *buff = malloc(buff_size);
	bzero(buff, buff_size);

	uint32_t benchmark_buffer_size = 256;
    uint8_t *benchmark_buffer = malloc(benchmark_buffer_size);
	bzero(benchmark_buffer, benchmark_buffer_size);

	dprintf("Receiving msg0..\n");
	read(sockfd, buff, buff_size);
	receive_msg0(ctx, buff, buff_size);

	dprintf("Sending msg1..\n");
	uint32_t msg1_size = prepare_msg1(ctx, buff, buff_size);
	dprintf("msg1_size: %u\n", msg1_size);
	write(sockfd, buff, msg1_size);

	dprintf("Receiving msg2..\n");
	read(sockfd, buff, buff_size);
	receive_msg2(ctx, buff, buff_size, benchmark_buffer, benchmark_buffer_size);

#ifdef PROFILING_MESSAGES
	// We don't need to send msg3 in the context of this benchmark
	goto out;
#endif

	dprintf("Sending msg3..\n");
	prepare_msg3(ctx, buff, buff_size, benchmark_buffer, benchmark_buffer_size);
	write(sockfd, buff, buff_size);

#ifdef PROFILING_MESSAGES
out:
#endif
	
	printf("%s", benchmark_buffer);

	free(benchmark_buffer);
	free(buff);
}

static void start_verifier_server(tee_ctx* ctx)
{
	int sockfd, connfd;
	socklen_t len;
    struct sockaddr_in servaddr, cli;
  
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        dprintf("Socket successfully created..\n");
	}

	/* setsockopt: Handy debugging trick that lets 
	* us rerun the server immediately after we kill it; 
	* otherwise we have to wait about 20 secs. 
	* Eliminates "ERROR on binding: Address already in use" error. 
	*/
  	int optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    bzero(&servaddr, sizeof(servaddr));
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
  
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        dprintf("Socket successfully binded..\n");
  
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(1);
    }
    else
        dprintf("Server listening..\n");
    len = sizeof(cli);
  
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server acccept failed...\n");
        exit(1);
    }
    else
        dprintf("server acccept the client...\n");
  
    // Function for chatting between client and server
    handle_connection(connfd, ctx);
  
    // After chatting close the socket
    close(sockfd);
}

static void terminate_tee_session(tee_ctx* ctx)
{
	TEEC_CloseSession(&ctx->sess);
	TEEC_FinalizeContext(&ctx->ctx);
}

int main(int argc, char *argv[])
{
#ifdef PROFILING_GENANN
	if (argc != 2) {
		dprintf("Error: the size of the iris dataset must be passed by argument.\n");
		exit(1);
	}

	int approximated_dataset_size_in_kb = atoi(argv[1]);
	secret_size = (approximated_dataset_size_in_kb + 10) * 1024; // Add 10KB for security)
	secret = malloc(secret_size);

	char filename[30];
	snprintf(filename, 30, "genann/iris-%d.data", approximated_dataset_size_in_kb);

	FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Could not open file: %s\n", filename);
        exit(1);
    }

    fgets((char*) secret, secret_size, f);
    fclose(f);
#else
	if (argc == 2) {
		// the additional argument is the size of a bootstrapped secret
		secret_size = atoi(argv[1]);
		secret = malloc(secret_size);
		memset(secret, 'A', secret_size);

		dprintf("Bootstrapping a secret of a custom length: %u.\n", secret_size);
	}
#endif

	tee_ctx ctx;

    dprintf("Prepare session with the TA\n");
    prepare_tee_session(&ctx);

	// For the benchmark, settings the secret is done from the outside of the TA
	dprintf("Bootstrap secret\n");
	bootstrap_secret(&ctx, secret, secret_size);

	start_verifier_server(&ctx);
  
	dprintf("Terminate session with the TA\n");
    terminate_tee_session(&ctx);

	if (argc == 2) {
		free(secret);
	}
  
    return 0;
}
