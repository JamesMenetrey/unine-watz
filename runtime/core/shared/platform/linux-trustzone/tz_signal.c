#include "platform_api_vmcore.h"

int raise(int sig)
{
    EMSG("raise is not supported.");
    return -1;
}