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

#include "mempool.h"
#include "base.h"
#include <string.h>

struct mem_pool_header {
  uint32_t bucket;
  uint32_t reserved;
  size_t allocation_size;
};

struct mem_pool_arena {
  struct mem_pool_arena *next;
  size_t block_size;
  size_t block_count;
  unsigned char data[];
};

static const size_t mem_pool_bucket_sizes[MEM_POOL_BUCKET_COUNT] = {
  32,
  64,
  128,
  256,
  512,
  1024,
};

static size_t mem_pool_header_size(void)
{
  return sizeof(struct mem_pool_header);
}

static size_t mem_pool_arena_capacity(size_t block_size)
{
  const size_t page_size = 4096;
  const size_t full_block = block_size + mem_pool_header_size();
  size_t capacity = page_size / full_block;

  if (capacity < 4)
    capacity = 4;

  if (capacity > 256)
    capacity = 256;

  return capacity;
}

static size_t mem_pool_bucket_index(size_t size)
{
  for (size_t i = 0; i < MEM_POOL_BUCKET_COUNT; i++) {
    if (size <= mem_pool_bucket_sizes[i])
      return i;
  }

  return (size_t)-1;
}

static void *mem_pool_allocate_block(struct mem_pool_fixed *pool,
                                     size_t bucket_index)
{
  size_t object_size = pool->object_size;
  size_t header_size = mem_pool_header_size();
  size_t full_block = object_size + header_size;
  size_t capacity = pool->arena_capacity;
  if (!capacity)
    capacity = mem_pool_arena_capacity(object_size);
  size_t arena_size = sizeof(struct mem_pool_arena) + full_block * capacity;
  struct mem_pool_arena *arena = bmalloc(arena_size);
  unsigned char *raw;

  arena->next = pool->arena_head;
  arena->block_size = object_size;
  arena->block_count = capacity;
  pool->arena_head = arena;

  raw = arena->data;
  pool->free_list = (void *)(raw + header_size);

  for (size_t i = 0; i < capacity; i++) {
    unsigned char *block = raw + i * full_block;
    struct mem_pool_header *header = (struct mem_pool_header *)block;
    void *payload = block + header_size;
    header->bucket = (uint32_t)bucket_index;
    header->allocation_size = object_size;
    if (i + 1 < capacity)
      *(void **)payload = raw + (i + 1) * full_block + header_size;
    else
      *(void **)payload = NULL;
  }

  return mem_pool_fixed_alloc(pool);
}

void mem_pool_fixed_init(struct mem_pool_fixed *pool,
                         size_t object_size,
                         size_t arena_capacity)
{
  if (!object_size)
    object_size = 1;

  pool->object_size = object_size;
  pool->arena_capacity = arena_capacity;
  pool->free_list = NULL;
  pool->arena_head = NULL;
}

void mem_pool_fixed_destroy(struct mem_pool_fixed *pool)
{
  struct mem_pool_arena *arena = pool->arena_head;

  while (arena) {
    struct mem_pool_arena *next = arena->next;
    bfree(arena);
    arena = next;
  }

  pool->free_list = NULL;
  pool->arena_head = NULL;
  pool->object_size = 0;
  pool->arena_capacity = 0;
}

void *mem_pool_fixed_alloc(struct mem_pool_fixed *pool)
{
  if (!pool->free_list)
    return mem_pool_allocate_block(pool, UINT32_MAX);

  void *ptr = pool->free_list;
  pool->free_list = *(void **)ptr;
  return ptr;
}

void mem_pool_fixed_free(struct mem_pool_fixed *pool, void *ptr)
{
  if (!ptr)
    return;

  *(void **)ptr = pool->free_list;
  pool->free_list = ptr;
}

void mem_pool_init(struct mem_pool *pool)
{
  for (size_t i = 0; i < MEM_POOL_BUCKET_COUNT; i++)
    mem_pool_fixed_init(&pool->buckets[i], mem_pool_bucket_sizes[i], 0);

  pool->page_size = 4096;
}

void mem_pool_destroy(struct mem_pool *pool)
{
  for (size_t i = 0; i < MEM_POOL_BUCKET_COUNT; i++)
    mem_pool_fixed_destroy(&pool->buckets[i]);

  pool->page_size = 0;
}

static void *mem_pool_alloc_from_bucket(struct mem_pool *pool, size_t bucket_index)
{
  struct mem_pool_fixed *bucket = &pool->buckets[bucket_index];
  if (!bucket->free_list)
    return mem_pool_allocate_block(bucket, bucket_index);

  void *ptr = bucket->free_list;
  bucket->free_list = *(void **)ptr;
  return ptr;
}

void *mem_pool_alloc(struct mem_pool *pool, size_t size)
{
  if (!size)
    size = 1;

  size_t bucket_index = mem_pool_bucket_index(size);

  if (bucket_index != (size_t)-1)
    return mem_pool_alloc_from_bucket(pool, bucket_index);

  size_t header_size = mem_pool_header_size();
  struct mem_pool_header *header = bmalloc(size + header_size);
  header->bucket = UINT32_MAX;
  header->allocation_size = size;
  return (unsigned char *)header + header_size;
}

void mem_pool_free(struct mem_pool *pool, void *ptr)
{
  if (!ptr)
    return;

  size_t header_size = mem_pool_header_size();
  struct mem_pool_header *header = (struct mem_pool_header *)((unsigned char *)ptr - header_size);

  if (header->bucket != UINT32_MAX) {
    size_t bucket_index = header->bucket;
    if (bucket_index < MEM_POOL_BUCKET_COUNT) {
      struct mem_pool_fixed *bucket = &pool->buckets[bucket_index];
      *(void **)ptr = bucket->free_list;
      bucket->free_list = ptr;
      return;
    }
  }

  bfree(header);
}
