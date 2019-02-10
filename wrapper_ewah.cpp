#include "wrapper.h"
#include "ewah.h"

typedef EWAHBoolArray<uint64_t> bitmap;

wrapped_bitmap_t *wrapped_bitmap_create()
{
	return reinterpret_cast<void*>(new bitmap);	
}

void wrapped_bitmap_add(wrapped_bitmap_t *a, uint32_t x)
{
	reinterpret_cast<bitmap*>(a)->set(x);
}

void wrapped_bitmap_free(wrapped_bitmap_t *a)
{
	delete reinterpret_cast<bitmap*>(a);	
}

wrapped_bitmap_t *wrapped_bitmap_and(wrapped_bitmap_t *a, wrapped_bitmap_t *b)
{
	bitmap *c = new bitmap;
	reinterpret_cast<bitmap*>(a)->logicaland(*(reinterpret_cast<bitmap*>(b)), *c);
	return c;
}

long wrapped_bitmap_get_cardinality(wrapped_bitmap_t *a)
{
	return reinterpret_cast<bitmap*>(a)->numberOfOnes();	
}
