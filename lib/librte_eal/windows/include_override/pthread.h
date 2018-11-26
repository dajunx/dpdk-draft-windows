/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#define PTHREAD_BARRIER_SERIAL_THREAD TRUE

typedef void* pthread_t;
typedef void pthread_attr_t;
typedef SYNCHRONIZATION_BARRIER pthread_barrier_t;
typedef HANDLE pthread_mutex_t;

#define pthread_barrier_init(barrier,attr,count) InitializeSynchronizationBarrier(barrier,count,-1)
#define pthread_barrier_wait(barrier) EnterSynchronizationBarrier(barrier,SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY)
#define pthread_barrier_destroy(barrier) DeleteSynchronizationBarrier(barrier)  
#define pthread_cancel(thread) TerminateThread(thread,0)
#define pthread_mutex_lock(mutex) WaitForSingleObject(mutex,INFINITE)
#define pthread_mutex_unlock(mutex) ReleaseMutex(mutex)
#define pthread_cond_signal(condition_variable) WakeConditionVariable(condition_variable)


// pthread function overrides
#define pthread_self()                                          ((pthread_t)GetCurrentThreadId())
#define pthread_setaffinity_np(thread,size,cpuset)              WinSetThreadAffinityMask(thread, cpuset)
#define pthread_getaffinity_np(thread,size,cpuset)              WinGetThreadAffinityMask(thread, cpuset)
#define pthread_create(threadID, threadattr, threadfunc, args)  WinCreateThreadOverride(threadID, threadattr, threadfunc, args)

typedef int pid_t;
pid_t fork(void);

static inline int WinSetThreadAffinityMask(void* threadID, unsigned long *cpuset)
{
	DWORD dwPrevAffinityMask = SetThreadAffinityMask(threadID, *cpuset);
	return 0;
}

static inline int WinGetThreadAffinityMask(void* threadID, unsigned long *cpuset)
{
	/* Workaround for the lack of a GetThreadAffinityMask() API in Windows */
	DWORD dwPrevAffinityMask = SetThreadAffinityMask(threadID, 0x1); /* obtain previous mask by setting dummy mask */
	SetThreadAffinityMask(threadID, dwPrevAffinityMask); /* set it back! */
	*cpuset = dwPrevAffinityMask;
	return 0;
}

static inline int WinCreateThreadOverride(void* threadID, const void* threadattr, void* threadfunc, void* args)
{
	HANDLE hThread;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfunc, args, 0, (LPDWORD)threadID);
	if (hThread)
	{
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
		SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	}
	return ((hThread != NULL) ? 0 : E_FAIL);
}

static inline int pthread_join(void* thread, void **value_ptr)
{
	return 0;
}

#endif /* _PTHREAD_H_ */
