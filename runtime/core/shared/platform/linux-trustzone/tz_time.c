#include "platform_api_vmcore.h"

static inline uint32_t tee_time_to_ms(TEE_Time t)
{
	return t.seconds * 1000 + t.millis;
}

/*
 * Provides the number of microseconds since midnight on January 1, 1970, UTC.
 * While this function should return the time since the REE started, this function
 * is mostly used for code profiling in the runtime (i.e., used for time difference),
 * so it is assumed ok to provide the UNIX timestamp.
 * 
 * Since the original attempt is to return the time since the REE started, the value
 * returned by this function is by essence untrusted.
 */
uint64 os_time_get_boot_microsecond()
{
    TEE_Time time;
    TEE_GetREETime(&time);

    return tee_time_to_ms(time) * 1000;
}


/*
 * WASI implementation
 */

#ifndef SGX_DISABLE_WASI

int clock_getres(int clock_id, struct timespec *res)
{
    EMSG("clock_getres is not supported.");
    return -1;
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    TEE_Time time;

    // The implemnentation has been changed in the kernel driver to retrieve the monotonic time
    if (clock_id != CLOCK_MONOTONIC) {
        EMSG("clock_gettime does not support a clock_id different from CLOCK_MONOTONIC.");
        return -1;
    }

    TEE_GetREETime(&time);

    tp->tv_sec = time.seconds;
    tp->tv_nsec = time.nanos; // The GP API has been extended to retrieve nano time

    return 0;
}

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain)
{
    EMSG("clock_nanosleep is not supported.");
    return -1;
}

int futimens(int fd, const struct timespec times[2])
{
    EMSG("futimens is not supported.");
    return -1;
}

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
    EMSG("utimensat is not supported.");
    return -1;
}

#endif