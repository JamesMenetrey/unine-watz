#include "platform_api_vmcore.h"


int os_cond_destroy(korp_cond *cond)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int os_cond_init(korp_cond *cond)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int os_mutex_destroy(korp_mutex *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int os_mutex_init(korp_mutex *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int os_mutex_lock(korp_mutex *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int os_mutex_unlock(korp_mutex *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
korp_tid os_self_thread()
{
    //TA is not multithreaded, returns the default value.
    return NULL;
}
uint8 *os_thread_get_stack_boundary()
{
    // Does not provide any insight of thread stack as the other functions are not implemented.
    return NULL;
}
