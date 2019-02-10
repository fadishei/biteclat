#ifndef BITSET_H
#define BITSET_H

#include "wrapper.h"
#include "itemset.h"

// a warpper around various bitset implementations
typedef struct
{
	long card;
	wrapped_bitmap_t *bitmap;
} bitset_t;

typedef struct
{
	int len;
	bitset_t *bitsets;
} bitset_bag_t;

bitset_bag_t *bitset_bag_create(itemset_bag_t *ibag);
void bitset_free(bitset_t *set);
void bitset_bag_free(bitset_bag_t *bag);

#endif
