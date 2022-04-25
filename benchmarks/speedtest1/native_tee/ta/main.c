#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <mwe_ta.h>

#include "sqlite3.h"

int bm_main();

TEE_Result TA_CreateEntryPoint(void) {
  DMSG("has been called");
  return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void) {
  DMSG("has been called");
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx) {
    uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE,
					     TEE_PARAM_TYPE_NONE);

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    (void)&params;
    (void)&sess_ctx;

    return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx) {
    (void)&sess_ctx;
}

static sqlite3* db;

inline static void error_check_with_msg(int status, char* err_msg)
{
    if (status != SQLITE_OK)
    {
        DMSG("SQL error: %s (status %d)\n", err_msg, status);
        sqlite3_free(err_msg);
    }
}

inline static void error_check(int status)
{
    if (status != SQLITE_OK)
    {
        DMSG("SQL error: status: %d\n", status);
    }
}

inline static void execute(const char* query)
{
    char* err_msg = NULL;
    int status;
    
    status = sqlite3_exec(db, query, NULL, NULL, &err_msg);
    error_check_with_msg(status, err_msg);
}

static int callback(void *notUsed, int argc, char **argv, char **azColName){
    (void)&notUsed;
    int i;

    for(i=0; i<argc; i++){
        DMSG("%s = %s; ", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    return 0;
}

static void TA_Hello(TEE_Param *param_time) {
    (void)&param_time;
    void* database_memory = TEE_Malloc(512 * 1024, 0);
    char *zErrMsg = 0;

    DMSG("Starting SQLite %s in memory.\n", SQLITE_VERSION);
    int status = sqlite3_config(SQLITE_CONFIG_HEAP, database_memory, 512 * 1024, 2);
    if (status)
    {
        DMSG("Can't configure the heap buffer of SQLite. status.");
        return;
    }

    status = sqlite3_open(":memory:", &db);
    if (status)
    {
        DMSG("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }

    DMSG("Creating a table..\n");
    status = sqlite3_exec(db, "CREATE TABLE person(firstname VARCHAR(10), lastname VARCHAR(10));", callback, 0, &zErrMsg);
    if(status != SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    DMSG("Inserting data..\n");
    status = sqlite3_exec(db, "INSERT INTO person VALUES('James', 'Menetrey');", callback, 0, &zErrMsg);
    if( status!=SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    status = sqlite3_exec(db, "INSERT INTO person VALUES('John', 'Smith');", callback, 0, &zErrMsg);
    if( status!=SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    DMSG("Querying the table..\n");
    sqlite3_exec(db, "SELECT * FROM person;", callback, 0, &zErrMsg);
    if( status!=SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    DMSG("Querying the UTC time..\n");
    sqlite3_exec(db, "SELECT datetime('now','localtime') AS current_time;", callback, 0, &zErrMsg);
    if( status!=SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    DMSG("Querying the table..\n");
    sqlite3_exec(db, "SELECT * FROM person;", callback, 0, &zErrMsg);
    if( status!=SQLITE_OK ){
        DMSG("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    }

    status = sqlite3_close(db);
    if (status != SQLITE_OK)
    {
        DMSG("Can't open database: %s\n", sqlite3_errmsg(db));
    }

    TEE_Free(database_memory);
}

static void TA_RunSpeedtest1(TEE_Param *param_time, char* benchmark_buffer, int benchmark_buffer_size) {
    bm_main(benchmark_buffer, benchmark_buffer_size);
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4]) {
    (void)&sess_ctx;
    (void)&param_types;
    (void)&params;

    switch (cmd_id) {
    case TA_COMMAND_HELLO:
        TA_Hello(&params[0]);
        return TEE_SUCCESS;
    case TA_COMMAND_RUN_SPEEDTEST1:
        TA_RunSpeedtest1(&params[0], params[0].memref.buffer, params[0].memref.size);
        return TEE_SUCCESS;
    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }
}
