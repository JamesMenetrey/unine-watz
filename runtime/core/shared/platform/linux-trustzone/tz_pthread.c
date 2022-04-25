#include "platform_api_vmcore.h"

/*
 * Errno APIs
 */

static int global_errno;

/*
 * Errno is not bound to a thread.
 */
int* __errno_location(void)
{
    return &global_errno;
}


/*
 * Cond APIs
 */
int pthread_cond_destroy(pthread_cond_t *cond)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_cond_init(pthread_cond_t *cond, const void *attr)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_cond_signal(pthread_cond_t *cond)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, uint64_t useconds)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}


/*
 * Mutex APIs
 */

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}


/* 
 * Rwlock APIs
 */

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *rwlock, void *attr)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    // TA is not multithreaded, mutexes are therefore not implemented.
    return 0;
}