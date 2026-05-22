/*
 * Copyright (c) 2026 Open Log Stream Analysis
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

#include "bmem.h"
#include "c99defs.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mem_pool_bucket_index {
  MEM_POOL_BUCKET_32 = 0,
  MEM_POOL_BUCKET_64,
  MEM_POOL_BUCKET_128,
  MEM_POOL_BUCKET_256,
  MEM_POOL_BUCKET_512,
  MEM_POOL_BUCKET_1024,
  MEM_POOL_BUCKET_COUNT,
};

struct mem_pool_fixed;
struct mem_pool;

struct mem_pool_fixed {
  size_t object_size;
  size_t arena_capacity;
  void *free_list;
  void *arena_head;
};

struct mem_pool {
  struct mem_pool_fixed buckets[MEM_POOL_BUCKET_COUNT];
  size_t page_size;
};

EXPORT void mem_pool_fixed_init(struct mem_pool_fixed *pool,
                                size_t object_size,
                                size_t arena_capacity);
EXPORT void mem_pool_fixed_destroy(struct mem_pool_fixed *pool);
EXPORT void *mem_pool_fixed_alloc(struct mem_pool_fixed *pool);
EXPORT void mem_pool_fixed_free(struct mem_pool_fixed *pool, void *ptr);

EXPORT void mem_pool_init(struct mem_pool *pool);
EXPORT void mem_pool_destroy(struct mem_pool *pool);
EXPORT void *mem_pool_alloc(struct mem_pool *pool, size_t size);
EXPORT void mem_pool_free(struct mem_pool *pool, void *ptr);

#ifdef __cplusplus
}
#endif
