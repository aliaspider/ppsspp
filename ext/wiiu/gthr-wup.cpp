
#include <bits/gthr.h>
#include <wiiu/os/debug.h>
#include <string.h>
#include <wiiu/mem.h>
#include <errno.h>

void __gthread_mutex_init(__gthread_mutex_t *mutex) {
	memset(mutex, 0, sizeof(mutex));
	OSInitMutex(mutex);
}

// TODO: not thread safe
int __gthread_once(__gthread_once_t *once, void (*func)()) {
	DEBUG_LINE();
	if (!once->is_initialized)
		return -1;

	if (once->init_executed)
		return 0;

	func();
	once->init_executed = 1;
	return 0;
}

int __gthread_key_create(__gthread_key_t *keyp, void (*dtor)(void *)) {
	DEBUG_LINE();
	return 0;
}

int __gthread_key_delete(__gthread_key_t key) {
	DEBUG_LINE();
	return 0;
}

void *__gthread_getspecific(__gthread_key_t key) {
	DEBUG_LINE();
	return NULL;
}

int __gthread_setspecific(__gthread_key_t key, const void *ptr) {
	DEBUG_LINE();
	return 0;
}

int __gthread_mutex_destroy(__gthread_mutex_t *mutex) {
	memset(mutex, 0, sizeof(*mutex));
	return 0;
}
int __gthread_recursive_mutex_destroy(__gthread_recursive_mutex_t *mutex) {
	memset(mutex, 0, sizeof(*mutex));
	return 0;
}

int __gthread_mutex_lock(__gthread_mutex_t *mutex) {
	OSLockMutex(mutex);
	return 0;
}
int __gthread_mutex_trylock(__gthread_mutex_t *mutex) {
	if (!OSTryLockMutex(mutex))
		return -1;
	return 0;
}
int __gthread_mutex_unlock(__gthread_mutex_t *mutex) {
	OSUnlockMutex(mutex);
	return 0;
}

int __gthread_recursive_mutex_lock(__gthread_recursive_mutex_t *mutex) {
	OSLockMutex(mutex);
	return 0;
}
int __gthread_recursive_mutex_trylock(__gthread_recursive_mutex_t *mutex) {
	if (!OSTryLockMutex(mutex))
		return -1;
	return 0;
}

int __gthread_recursive_mutex_unlock(__gthread_recursive_mutex_t *mutex) {
	OSUnlockMutex(mutex);
	return 0;
}

void __gthread_cond_init(__gthread_cond_t *cond) {
	OSInitCond(cond);
}

int __gthread_cond_destroy (__gthread_cond_t* cond){
	memset(cond, 0, sizeof(*cond));
	return 0;
}
int __gthread_cond_broadcast(__gthread_cond_t *cond) {
	OSSignalCond(cond);
	return 0;
}
int __gthread_cond_wait(__gthread_cond_t *cond, __gthread_mutex_t *mutex) {
	OSWaitCond(cond, mutex);
	return 0;
}
int __gthread_cond_wait_recursive(__gthread_cond_t *cond, __gthread_recursive_mutex_t *mutex) {
	OSWaitCond(cond, mutex);
	return 0;
}

static int OSThreadEntryPointFnWrap(int argc, const char **argv) {
	void *(*func)(void *) = (void *(*)(void *))(argv[0]);
	void *args = (void *)argv[1];
	return (int)func(args);
}

int __gthread_create(__gthread_t *_thread, void *(*func)(void *), void *args) {
	OSThreadAttributes attr = OS_THREAD_ATTRIB_AFFINITY_ANY;
	OSThread *thread = (OSThread *)MEMAllocFromExpHeapEx((MEMExpandedHeap*)MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), sizeof(OSThread) + __GTHREADS_STACK_SIZE + 16, 8);
	memset(thread, 0, sizeof(OSThread));
	u8 *stack = (u8 *)thread + sizeof(OSThread);
	void **argv = (void**)(stack + __GTHREADS_STACK_SIZE);
	argv[0] = (void *)func;
	argv[1] = args;
	argv[2] = NULL;

	if (!OSCreateThread(thread, OSThreadEntryPointFnWrap, 2, (char *)argv, stack + __GTHREADS_STACK_SIZE, __GTHREADS_STACK_SIZE, OS_THREAD_DEF_PRIO, attr)) {
		MEMFreeToExpHeap((MEMExpandedHeap*)MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), thread);
		return EINVAL;
	}

	OSResumeThread(thread);
	*_thread = thread;

	return 0;
}

int __gthread_join(__gthread_t thread, void **value_ptr) {
	if (!OSJoinThread(thread, (int *)value_ptr))
		return EINVAL;
	MEMFreeToExpHeap((MEMExpandedHeap*)MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM2), thread);
	return 0;
}

int __gthread_detach(__gthread_t thread) {
	OSDetachThread(thread);
	return 0;
}

int __gthread_equal(__gthread_t t1, __gthread_t t2) {
	return t1->id == t2->id;
}

__gthread_t __gthread_self(void) {
	return OSGetCurrentThread();
}

int __gthread_yield(void) {
	OSYieldThread();
	return 0;
}

int __gthread_mutex_timedlock(__gthread_mutex_t *m, const __gthread_time_t *abs_timeout) {
	OSLockMutex(m);
	return 0;
}

int __gthread_recursive_mutex_timedlock(__gthread_recursive_mutex_t *m, const __gthread_time_t *abs_time) {
	OSLockMutex(m);
	return 0;
}

int __gthread_cond_signal(__gthread_cond_t *cond) {
	OSSignalCond(cond);
	return 0;
}

int __gthread_cond_timedwait(__gthread_cond_t *cond, __gthread_mutex_t *mutex, const __gthread_time_t *abs_timeout) {
	OSWaitCond(cond, mutex);
	return 0;
}
