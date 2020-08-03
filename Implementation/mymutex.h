#ifndef MYMUTEX_H
#define MYMUTEX_H
#define _GNU_SOURCE
#include<linux/futex.h>
#include<sys/time.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<pthread.h>
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P) __sync_add_and_fetch((P), 1)
#define atomic_dec(P) __sync_add_and_fetch((P), -1) 
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V) __sync_or_and_fetch((P), 1<<(V))
#define atomic_clear_bit(P, V) __sync_and_and_fetch((P), ~(1<<(V)))
#define cpu_relax()  __asm__ __volatile__ ( "pause" : : : )
/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */ 
/*#define cpu_relax() asm volatile("pause\n": : :"memory")*/

/* Atomic exchange (of various sizes) */
static inline void *xchg_64(void *ptr, void *x)
{
	__asm__ __volatile__("xchgq %0,%1"
				:"=r" ((unsigned long long) x)
				:"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
				:"memory");

	return x;
}

static inline unsigned xchg_32(void *ptr, unsigned x)
{
	__asm__ __volatile__("xchgl %0,%1"
				:"=r" ((unsigned) x)
				:"m" (*(volatile unsigned *)ptr), "0" (x)
				:"memory");

	return x;
}

static inline unsigned short xchg_16(void *ptr, unsigned short x)
{
	__asm__ __volatile__("xchgw %0,%1"
				:"=r" ((unsigned short) x)
				:"m" (*(volatile unsigned short *)ptr), "0" (x)
				:"memory");

	return x;
}

/* Test and set a bit */
static inline char atomic_bitsetandtest(void *ptr, int x)
{
	char out;
	__asm__ __volatile__("lock; bts %2,%1\n"
						"sbb %0,%0\n"
				:"=r" (out), "=m" (*(volatile long long *)ptr)
				:"Ir" (x)
				:"memory");

	return out;
}

typedef int mutex;

#define MUTEX_INITIALIZER {0}
int sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

int mutex_init(mutex *m, const pthread_mutexattr_t *a)
{
	(void) a;
	*m = 0;
	return 0;
}

int mutex_destroy(mutex *m)
{
	/* Do nothing */
	(void) m;
	return 0;
}
int mutex_lock(mutex *m)
{
	int i, c;
	
	/* Spin and try to take lock */
	for (i = 0; i < 100; i++)
	{
		c = cmpxchg(m, 0, 1);
		if (!c) return 0;
		
		cpu_relax();
	}

	/* The lock is now contended */
	if (c == 1) c = xchg_32(m, 2);

	while (c)
	{
		/* Wait in the kernel */
		sys_futex(m, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
		c = xchg_32(m, 2);
	}
	
	return 0;
}

int mutex_unlock(mutex *m)
{
	int i;
	
	/* Unlock, and if not contended then exit. */
	if (*m == 2)
	{
		*m = 0;
	}
	else if (xchg_32(m, 0) == 1) return 0;

	/* Spin and hope someone takes the lock */
	for (i = 0; i < 200; i++)
	{
		if (*m)
		{
			/* Need to set to state 2 because there may be waiters */
			if (cmpxchg(m, 1, 2)) return 0;
		}
		cpu_relax();
	}
	
	/* We need to wake someone up */
	sys_futex(m, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
	
	return 0;
}
#endif
