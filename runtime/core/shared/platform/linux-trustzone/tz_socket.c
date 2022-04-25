#include "platform_api_vmcore.h"

int socket(int domain, int type, int protocol)
{
    EMSG("socket is not supported.");
    return -1;
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    EMSG("getsockopt is not supported.");
    return -1;
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    EMSG("sendmsg is not supported.");
    return -1;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    EMSG("recvmsg is not supported.");
    return -1;
}

int shutdown(int sockfd, int how)
{
    EMSG("shutdown is not supported.");
    return -1;
}
