#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define timespec_to_micro(t) \
    t.tv_sec * 1000 * 1000 + t.tv_nsec / 1000

int main(int argc, char **argv)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);

    printf("%lld\n", timespec_to_micro(time));

    return 0;
}
