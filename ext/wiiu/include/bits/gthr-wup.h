
#ifndef _GLIBCXX_GCC_GTHR_WUP_H
#define _GLIBCXX_GCC_GTHR_WUP_H

#include <wiiu/os/thread.h>
#include <wiiu/os/mutex.h>
#include <wiiu/os/condition.h>

#define _GLIBCXX_HAS_GTHREADS 1
#define __GTHREADS 1
#define __GTHREAD_HAS_COND 1
#define __GTHREADS_CXX0X 1
#define __GTHREADS_STACK_SIZE (2 * 1024 * 1024)
//#define _UNIX98_THREAD_MUTEX_ATTRIBUTES 1
//#define _GTHREAD_USE_MUTEX_TIMEDLOCK 0

#define __GTHREAD_COND_INIT_FUNCTION __gthread_cond_init
#define __GTHREAD_MUTEX_INIT_FUNCTION __gthread_mutex_init
#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION __gthread_mutex_init
//#define __GTHREAD_COND_INIT_FUNCTION OSInitCond
//#define __GTHREAD_MUTEX_INIT_FUNCTION OSInitMutex
//#define __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION OSInitMutex

#define __GTHREAD_ONCE_INIT	{ 1, 0 }

//#define __GTHREAD_MUTEX_INIT {}
//#define __GTHREAD_RECURSIVE_MUTEX_INIT {}
//#define __GTHREAD_COND_INIT {}

typedef struct {
	int is_initialized;
	int init_executed;
} __gthread_once_t;

typedef OSMutex __gthread_mutex_t;
typedef OSMutex __gthread_recursive_mutex_t;
typedef OSCondition __gthread_cond_t;
typedef OSThread *__gthread_t;
typedef void *__gthread_key_t;
typedef struct timespec __gthread_time_t;

static inline int __gthread_active_p(void) { return 1; }

void __gthread_mutex_init(__gthread_mutex_t *mutex);
int __gthread_once(__gthread_once_t *once, void (*func)());
int __gthread_key_create(__gthread_key_t *keyp, void (*dtor)(void *));
int __gthread_key_delete(__gthread_key_t key);
void *__gthread_getspecific(__gthread_key_t key);
int __gthread_setspecific(__gthread_key_t key, const void *ptr);
int __gthread_mutex_destroy(__gthread_mutex_t *mutex);
int __gthread_recursive_mutex_destroy(__gthread_recursive_mutex_t *mutex);
int __gthread_mutex_lock(__gthread_mutex_t *mutex);
int __gthread_mutex_trylock(__gthread_mutex_t *mutex);
int __gthread_mutex_unlock(__gthread_mutex_t *mutex);
int __gthread_recursive_mutex_lock(__gthread_recursive_mutex_t *mutex);
int __gthread_recursive_mutex_trylock(__gthread_recursive_mutex_t *mutex);
int __gthread_recursive_mutex_unlock(__gthread_recursive_mutex_t *mutex);
void __gthread_cond_init(__gthread_cond_t *cond);
int __gthread_cond_destroy (__gthread_cond_t* cond);
int __gthread_cond_broadcast(__gthread_cond_t *cond);
int __gthread_cond_wait(__gthread_cond_t *cond, __gthread_mutex_t *mutex);
int __gthread_cond_wait_recursive(__gthread_cond_t *cond, __gthread_recursive_mutex_t *mutex);
int __gthread_create(__gthread_t *_thread, void *(*func)(void *), void *args);
int __gthread_join(__gthread_t thread, void **value_ptr);
int __gthread_detach(__gthread_t thread);
int __gthread_equal(__gthread_t t1, __gthread_t t2);
__gthread_t __gthread_self(void);
int __gthread_yield(void);
int __gthread_mutex_timedlock(__gthread_mutex_t *m, const __gthread_time_t *abs_timeout);
int __gthread_recursive_mutex_timedlock(__gthread_recursive_mutex_t *m, const __gthread_time_t *abs_time);
int __gthread_cond_signal(__gthread_cond_t *cond);
int __gthread_cond_timedwait(__gthread_cond_t *cond, __gthread_mutex_t *mutex, const __gthread_time_t *abs_timeout);

#endif // _GLIBCXX_GCC_GTHR_WUP_H
