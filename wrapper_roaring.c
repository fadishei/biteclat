#include "wrapper.h"

wrapped_bitmap_t *wrapped_bitmap_create()
{
	return roaring_bitmap_create();
}

void wrapped_bitmap_add(wrapped_bitmap_t *a, uint32_t x)
{
	roaring_bitmap_add(a, x);
}

void wrapped_bitmap_free(wrapped_bitmap_t *a)
{
	return roaring_bitmap_free(a);
}

wrapped_bitmap_t *wrapped_bitmap_and(wrapped_bitmap_t *a, wrapped_bitmap_t *b)
{
	return roaring_bitmap_and(a, b);
}

long wrapped_bitmap_get_cardinality(wrapped_bitmap_t *a)
{
	return roaring_bitmap_get_cardinality(a);
}
