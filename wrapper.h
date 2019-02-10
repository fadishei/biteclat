#ifndef WRAPPER_H
#define WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define ROARING	1
#define EWAH	2
#define BM		3
#define CONCISE	4


#if BITSET==ROARING
#include "roaring.h"
typedef roaring_bitmap_t wrapped_bitmap_t;
#else
typedef void wrapped_bitmap_t;
#endif

wrapped_bitmap_t *wrapped_bitmap_create();
void wrapped_bitmap_free(wrapped_bitmap_t *a);
void wrapped_bitmap_add(wrapped_bitmap_t *a, uint32_t x);
wrapped_bitmap_t *wrapped_bitmap_and(wrapped_bitmap_t *a, wrapped_bitmap_t *b);
long wrapped_bitmap_get_cardinality(wrapped_bitmap_t *a);

#ifdef __cplusplus
}
#endif

#endif