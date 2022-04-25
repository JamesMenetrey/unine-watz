#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <mwe_ta.h>

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

# define teetime_to_micro(t) \
    (long int)t.seconds * 1000 * 1000 + (long int)t.nanos / 1000

static void TA_Roundtrip(TEE_Param *param_time) {
    TEE_Time time;
    TEE_GetREETime(&time);

    param_time->value.a = time.seconds;
    param_time->value.b = time.nanos;
}

static void TA_GetTime(TEE_Param *delta) {
    TEE_Time start, end, dest;

    TEE_GetREETime(&start);
    TEE_GetREETime(&end);

    TEE_TIME_NANOS_SUB(end, start, dest);
    
    delta->value.a = dest.seconds;
    delta->value.b = dest.nanos;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4]) {
    (void)&sess_ctx;
    (void)&param_types;
    (void)&params;

    switch (cmd_id) {
    case TA_COMMAND_ROUNDTRIP:
        TA_Roundtrip(&params[0]);
        return TEE_SUCCESS;
    case TA_COMMAND_GETREETIME:
        TA_GetTime(&params[0]);
        return TEE_SUCCESS;
    default:
        return TEE_ERROR_BAD_PARAMETERS;
    }
}
