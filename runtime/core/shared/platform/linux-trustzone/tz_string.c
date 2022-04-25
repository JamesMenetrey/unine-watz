#include "platform_api_vmcore.h"

size_t strcspn(const char *s1, const char *s2)
{
    EMSG("strcspn is not supported.");
    return -1;
}

char* strerror(int n)
{
    EMSG("strerror is not supported.");
    return NULL;
}

size_t strspn(const char *s, const char *accept)
{
    EMSG("strspn is not supported.");
    return -1;
}