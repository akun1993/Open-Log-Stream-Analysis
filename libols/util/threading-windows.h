/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <intrin.h>
#include <string.h>

#if !defined(_M_IX86) && !defined(_M_X64) && !defined(_M_ARM) && \
	!defined(_M_ARM64)
#error Processor not supported
#endif

static inline long os_atomic_inc_long(volatile long *val)
{
	return _InterlockedIncrement(val);
}

static inline long os_atomic_dec_long(volatile long *val)
{
	return _InterlockedDecrement(val);
}

static inline void os_atomic_store_long(volatile long *ptr, long val)
{
#if defined(_M_ARM64)
	_ReadWriteBarrier();
	__stlr32((volatile unsigned *)ptr, val);
	_ReadWriteBarrier();
#elif defined(_M_ARM)
	__dmb(_ARM_BARRIER_ISH);
	__iso_volatile_store32((volatile __int32 *)ptr, val);
	__dmb(_ARM_BARRIER_ISH);
#else
	_InterlockedExchange(ptr, val);
#endif
}

static inline long os_atomic_set_long(volatile long *ptr, long val)
{
	return _InterlockedExchange(ptr, val);
}

static inline long os_atomic_exchange_long(volatile long *ptr, long val)
{
	return os_atomic_set_long(ptr, val);
}

static inline long os_atomic_load_long(const volatile long *ptr)
{
#if defined(_M_ARM64)
	const long val = __ldar32((volatile unsigned *)ptr);
#else
	const long val = __iso_volatile_load32((const volatile __int32 *)ptr);
#endif

#if defined(_M_ARM)
	__dmb(_ARM_BARRIER_ISH);
#else
	_ReadWriteBarrier();
#endif

	return val;
}
