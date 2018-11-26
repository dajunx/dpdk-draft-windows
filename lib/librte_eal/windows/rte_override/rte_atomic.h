/* SPDX-License-Identifier: BSD-3-Clause
* Copyright(c) 2017-2018 Intel Corporation
*/

#ifndef _RTE_ATOMIC_H_
#define _RTE_ATOMIC_H_

#include <emmintrin.h>

/* Do not include any of the core rte_atomic.h includes. They cause compilation problems on Windows */
/* Instead, duplicate some of the required definitions here - this is sub-optimal, but... */

#define rte_mb() _mm_mfence()
#define rte_wmb() _mm_sfence()
#define rte_rmb() _mm_lfence()

#define rte_smp_mb() rte_mb()
#define rte_smp_rmb() _ReadBarrier()
#define rte_smp_wmb() _WriteBarrier()


#define rte_io_mb() rte_mb()
#define rte_io_wmb() _ReadBarrier()
#define rte_io_rmb() _WriteBarrier()

/**
* Compiler barrier.
*
* Guarantees that operation reordering does not occur at compile time
* for operations directly before and after the barrier.
*/
#define	rte_compiler_barrier() do {		\
	asm volatile ("" : : : "memory");	\
} while(0)


/* Inline Windows implementation of atomic operations */

/*------------------------- 16 bit atomic operations -------------------------*/

/**
* Atomic compare and set.
*
* (atomic) equivalent to:
*   if (*dst == exp)
*     *dst = src (all 16-bit words)
*
* @param dst
*   The destination location into which the value will be written.
* @param exp
*   The expected value.
* @param src
*   The new value.
* @return
*   Non-zero on success; 0 on failure.
*/
static inline int
rte_atomic16_cmpset(volatile uint16_t *dst, uint16_t exp, uint16_t src)
{
    return (_InterlockedCompareExchange16((SHORT *)dst, src, exp) != src);
}

/**
* The atomic counter structure.
*/
typedef struct {
    volatile int16_t cnt; /**< An internal counter value. */
} rte_atomic16_t;

/**
* Static initializer for an atomic counter.
*/
#define RTE_ATOMIC16_INIT(val) { (val) }

/**
* Initialize an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic16_init(rte_atomic16_t *v)
{
    v->cnt = 0;
}

/**
* Atomically read a 16-bit value from a counter.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   The value of the counter.
*/
static inline int16_t
rte_atomic16_read(const rte_atomic16_t *v)
{
    return v->cnt;
}

/**
* Atomically set a counter to a 16-bit value.
*
* @param v
*   A pointer to the atomic counter.
* @param new_value
*   The new value for the counter.
*/
static inline void
rte_atomic16_set(rte_atomic16_t *v, int16_t new_value)
{
    _InterlockedExchange16(&v->cnt, new_value);
}

/**
* Atomically add a 16-bit value to an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
*/
static inline void
rte_atomic16_add(rte_atomic16_t *v, int16_t inc)
{
    _InterlockedExchangeAdd16(&v->cnt, inc);
}

/**
* Atomically subtract a 16-bit value from an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
*/
static inline void
rte_atomic16_sub(rte_atomic16_t *v, int16_t dec)
{
    _InterlockedExchangeAdd16(&v->cnt, (-dec));
}

/**
* Atomically increment a counter by one.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic16_inc(rte_atomic16_t *v)
{
    rte_atomic16_add(v, 1);
}

/**
* Atomically decrement a counter by one.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic16_dec(rte_atomic16_t *v)
{
    rte_atomic16_sub(v, 1);
}

/**
* Atomically add a 16-bit value to a counter and return the result.
*
* Atomically adds the 16-bits value (inc) to the atomic counter (v) and
* returns the value of v after addition.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
* @return
*   The value of v after the addition.
*/
static inline int16_t
rte_atomic16_add_return(rte_atomic16_t *v, int16_t inc)
{
    _InterlockedExchangeAdd16(&v->cnt, inc);
    return v->cnt;
}

/**
* Atomically subtract a 16-bit value from a counter and return
* the result.
*
* Atomically subtracts the 16-bit value (inc) from the atomic counter
* (v) and returns the value of v after the subtraction.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
* @return
*   The value of v after the subtraction.
*/
static inline int16_t
rte_atomic16_sub_return(rte_atomic16_t *v, int16_t dec)
{
    _InterlockedExchangeAdd16(&v->cnt, (-dec));
    return v->cnt;
}

/**
* Atomically increment a 16-bit counter by one and test.
*
* Atomically increments the atomic counter (v) by one and returns true if
* the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after the increment operation is 0; false otherwise.
*/
static inline int rte_atomic16_inc_and_test(rte_atomic16_t *v)
{
    return ((rte_atomic16_add_return(v, 1) == 0));
}

/**
* Atomically decrement a 16-bit counter by one and test.
*
* Atomically decrements the atomic counter (v) by one and returns true if
* the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after the decrement operation is 0; false otherwise.
*/
static inline int rte_atomic16_dec_and_test(rte_atomic16_t *v)
{
    return ((rte_atomic16_sub_return(v, 1) == 0));
}

/**
* Atomically test and set a 16-bit atomic counter.
*
* If the counter value is already set, return 0 (failed). Otherwise, set
* the counter value to 1 and return 1 (success).
*
* @param v
*   A pointer to the atomic counter.
* @return
*   0 if failed; else 1, success.
*/
static inline int rte_atomic16_test_and_set(rte_atomic16_t *v)
{
    return rte_atomic16_cmpset((volatile uint16_t *)&v->cnt, 0, 1);
}

/**
* Atomically set a 16-bit counter to 0.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void rte_atomic16_clear(rte_atomic16_t *v)
{
    rte_atomic16_set(v, 0);
}

/*------------------------- 32 bit atomic operations -------------------------*/

/**
* Atomic compare and set.
*
* (atomic) equivalent to:
*   if (*dst == exp)
*     *dst = src (all 32-bit words)
*
* @param dst
*   The destination location into which the value will be written.
* @param exp
*   The expected value.
* @param src
*   The new value.
* @return
*   Non-zero on success; 0 on failure.
*/
static inline int
rte_atomic32_cmpset(volatile uint32_t *dst, uint32_t exp, uint32_t src)
{
    return (_InterlockedCompareExchange(dst, src, exp) != src);
}

/**
* The atomic counter structure.
*/
typedef struct {
    volatile int32_t cnt; /**< An internal counter value. */
} rte_atomic32_t;

/**
* Static initializer for an atomic counter.
*/
#define RTE_ATOMIC32_INIT(val) { (val) }

/**
* Initialize an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic32_init(rte_atomic32_t *v)
{
    v->cnt = 0;
}

/**
* Atomically read a 32-bit value from a counter.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   The value of the counter.
*/
static inline int32_t
rte_atomic32_read(const rte_atomic32_t *v)
{
    return v->cnt;
}

/**
* Atomically set a counter to a 32-bit value.
*
* @param v
*   A pointer to the atomic counter.
* @param new_value
*   The new value for the counter.
*/
static inline void
rte_atomic32_set(rte_atomic32_t *v, int32_t new_value)
{
    _InterlockedExchange((LONG volatile *)&v->cnt, new_value);
}

/**
* Atomically add a 32-bit value to an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
*/
static inline void
rte_atomic32_add(rte_atomic32_t *v, int32_t inc)
{
    _InterlockedExchangeAdd((LONG volatile *)&v->cnt, inc);
}

/**
* Atomically subtract a 32-bit value from an atomic counter.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
*/
static inline void
rte_atomic32_sub(rte_atomic32_t *v, int32_t dec)
{
    _InterlockedExchangeAdd((LONG volatile *)&v->cnt, (-dec));
}

/**
* Atomically increment a counter by one.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic32_inc(rte_atomic32_t *v)
{
    rte_atomic32_add(v, 1);
}

/**
* Atomically decrement a counter by one.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic32_dec(rte_atomic32_t *v)
{
    rte_atomic32_sub(v, 1);
}

/**
* Atomically add a 32-bit value to a counter and return the result.
*
* Atomically adds the 32-bits value (inc) to the atomic counter (v) and
* returns the value of v after addition.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
* @return
*   The value of v after the addition.
*/
static inline int32_t
rte_atomic32_add_return(rte_atomic32_t *v, int32_t inc)
{
    _InterlockedExchangeAdd((LONG volatile *)&v->cnt, inc);
    return v->cnt;
}

/**
* Atomically subtract a 32-bit value from a counter and return
* the result.
*
* Atomically subtracts the 32-bit value (inc) from the atomic counter
* (v) and returns the value of v after the subtraction.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
* @return
*   The value of v after the subtraction.
*/
static inline int32_t
rte_atomic32_sub_return(rte_atomic32_t *v, int32_t dec)
{
    _InterlockedExchangeAdd((LONG volatile *)&v->cnt, (-dec));
    return v->cnt;
}

/**
* Atomically increment a 32-bit counter by one and test.
*
* Atomically increments the atomic counter (v) by one and returns true if
* the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after the increment operation is 0; false otherwise.
*/
static inline int rte_atomic32_inc_and_test(rte_atomic32_t *v)
{
    return ((rte_atomic32_add_return(v, 1) == 0));
}

/**
* Atomically decrement a 32-bit counter by one and test.
*
* Atomically decrements the atomic counter (v) by one and returns true if
* the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after the decrement operation is 0; false otherwise.
*/
static inline int rte_atomic32_dec_and_test(rte_atomic32_t *v)
{
    return ((rte_atomic32_sub_return(v, 1) == 0));
}

/**
* Atomically test and set a 32-bit atomic counter.
*
* If the counter value is already set, return 0 (failed). Otherwise, set
* the counter value to 1 and return 1 (success).
*
* @param v
*   A pointer to the atomic counter.
* @return
*   0 if failed; else 1, success.
*/
static inline int rte_atomic32_test_and_set(rte_atomic32_t *v)
{
    return rte_atomic32_cmpset((volatile uint32_t *)&v->cnt, 0, 1);
}

/**
* Atomically set a 32-bit counter to 0.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void rte_atomic32_clear(rte_atomic32_t *v)
{
    rte_atomic32_set(v, 0);
}

/*------------------------- 64 bit atomic operations -------------------------*/

/**
* An atomic compare and set function used by the mutex functions.
* (atomic) equivalent to:
*   if (*dst == exp)
*     *dst = src (all 64-bit words)
*
* @param dst
*   The destination into which the value will be written.
* @param exp
*   The expected value.
* @param src
*   The new value.
* @return
*   Non-zero on success; 0 on failure.
*/
static inline int
rte_atomic64_cmpset(volatile uint64_t *dst, uint64_t exp, uint64_t src)
{
    return (_InterlockedCompareExchange64((volatile LONG64 *)dst, src, exp) != src);
}

/**
* Atomic exchange.
*
* (atomic)equivalent to :
*ret = *dst
*   *dst = val;
*return ret;
*
* @param dst
*   The destination location into which the value will be written.
* @param val
*   The new value.
* @return
*   The original value at that location
**/
static inline uint64_t
rte_atomic64_exchange(volatile uint64_t *dst, uint64_t val){
	return _InterlockedExchange64((volatile LONG64 *)dst, val);
}

/**
* The atomic counter structure.
*/
typedef struct {
    volatile int64_t cnt;  /**< Internal counter value. */
} rte_atomic64_t;

/**
* Static initializer for an atomic counter.
*/
#define RTE_ATOMIC64_INIT(val) { (val) }

/**
* Initialize the atomic counter.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic64_init(rte_atomic64_t *v)
{
    v->cnt = 0;
}

/**
* Atomically read a 64-bit counter.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   The value of the counter.
*/
static inline int64_t
rte_atomic64_read(rte_atomic64_t *v)
{
    return v->cnt;
}

/**
* Atomically set a 64-bit counter.
*
* @param v
*   A pointer to the atomic counter.
* @param new_value
*   The new value of the counter.
*/
static inline void
rte_atomic64_set(rte_atomic64_t *v, int64_t new_value)
{
    _InterlockedExchange64(&v->cnt, new_value);
}

/**
* Atomically add a 64-bit value to a counter.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
*/
static inline void
rte_atomic64_add(rte_atomic64_t *v, int64_t inc)
{
    _InterlockedExchangeAdd64(&v->cnt, inc);
}

/**
* Atomically subtract a 64-bit value from a counter.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
*/
static inline void
rte_atomic64_sub(rte_atomic64_t *v, int64_t dec)
{
    _InterlockedExchangeAdd64(&v->cnt, (-dec));
}

/**
* Atomically increment a 64-bit counter by one and test.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic64_inc(rte_atomic64_t *v)
{
    _InterlockedIncrement64(&v->cnt);
}

/**
* Atomically decrement a 64-bit counter by one and test.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void
rte_atomic64_dec(rte_atomic64_t *v)
{
    _InterlockedDecrement64(&v->cnt);
}

/**
* Add a 64-bit value to an atomic counter and return the result.
*
* Atomically adds the 64-bit value (inc) to the atomic counter (v) and
* returns the value of v after the addition.
*
* @param v
*   A pointer to the atomic counter.
* @param inc
*   The value to be added to the counter.
* @return
*   The value of v after the addition.
*/
static inline int64_t
rte_atomic64_add_return(rte_atomic64_t *v, int64_t inc)
{
    _InterlockedExchangeAdd64(&v->cnt, inc);
    return v->cnt;
}

/**
* Subtract a 64-bit value from an atomic counter and return the result.
*
* Atomically subtracts the 64-bit value (dec) from the atomic counter (v)
* and returns the value of v after the subtraction.
*
* @param v
*   A pointer to the atomic counter.
* @param dec
*   The value to be subtracted from the counter.
* @return
*   The value of v after the subtraction.
*/
static inline int64_t
rte_atomic64_sub_return(rte_atomic64_t *v, int64_t dec)
{
    _InterlockedExchangeAdd64(&v->cnt, (-dec));
    return v->cnt;
}

/**
* Atomically increment a 64-bit counter by one and test.
*
* Atomically increments the atomic counter (v) by one and returns
* true if the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after the addition is 0; false otherwise.
*/
static inline int rte_atomic64_inc_and_test(rte_atomic64_t *v)
{
    return ((rte_atomic64_add_return(v, 1) == 0));
}

/**
* Atomically decrement a 64-bit counter by one and test.
*
* Atomically decrements the atomic counter (v) by one and returns true if
* the result is 0, or false in all other cases.
*
* @param v
*   A pointer to the atomic counter.
* @return
*   True if the result after subtraction is 0; false otherwise.
*/
static inline int rte_atomic64_dec_and_test(rte_atomic64_t *v)
{
    return ((rte_atomic64_sub_return(v, 1) == 0));
}

/**
* Atomically test and set a 64-bit atomic counter.
*
* If the counter value is already set, return 0 (failed). Otherwise, set
* the counter value to 1 and return 1 (success).
*
* @param v
*   A pointer to the atomic counter.
* @return
*   0 if failed; else 1, success.
*/
static inline int rte_atomic64_test_and_set(rte_atomic64_t *v)
{
    return rte_atomic64_cmpset((volatile uint64_t *)&v->cnt, 0, 1);
}

/**
* Atomically set a 64-bit counter to 0.
*
* @param v
*   A pointer to the atomic counter.
*/
static inline void rte_atomic64_clear(rte_atomic64_t *v)
{
    rte_atomic64_set(v, 0);
}

#endif /* _RTE_ATOMIC_H_ */

/* */
/* */
