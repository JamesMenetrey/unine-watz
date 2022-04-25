#ifndef _TZ_PTHREAD_H
#define _TZ_PTHREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Errno APIs */
int* __errno_location(void);

/* Cond APIs */
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_init(pthread_cond_t *cond, const void *attr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, uint64_t useconds);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

/* Condattr APIs */
//int pthread_condattr_getclock(const pthread_condattr_t *restrict attr, clockid_t *restrict clock_id);
//int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);

/* Mutex APIs */
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

/* Rwlock APIs */
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_init(pthread_rwlock_t *rwlock, void *attr);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

#ifdef __cplusplus
}
#endif

#endif /* end of _TZ_PTHREAD_H */

