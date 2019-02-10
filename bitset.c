#include <stdlib.h>
#include "bitset.h"

bitset_bag_t *bitset_bag_create(itemset_bag_t *ibag)
{
	long i, j;
	
	bitset_bag_t *bbag;
	
	bbag = (bitset_bag_t *)malloc(sizeof(bitset_bag_t));
	if (!bbag)
		goto e1;
	
	bbag->len = 0;
	bbag->bitsets = (bitset_t *)malloc((ibag->item_max+1)*sizeof(bitset_t));
	if (!bbag->bitsets)
		goto e2;
	for (i=0; i<ibag->item_max+1; i++)
	{
		bbag->bitsets[i].bitmap = wrapped_bitmap_create();
		bbag->bitsets[i].card = 0;
		if (!bbag->bitsets[i].bitmap)
			goto e2;
		bbag->len++;
	}
	for (i=0; i<ibag->len; i++)
		for (j=0; j<ibag->itemsets[i].len; j++)
		{
			wrapped_bitmap_add(bbag->bitsets[ibag->itemsets[i].items[j]].bitmap, i);
			bbag->bitsets[ibag->itemsets[i].items[j]].card++;
		}
	return bbag;
	
e2:
	bitset_bag_free(bbag);
e1:
	return NULL;
}

void bitset_free(bitset_t *set)
{
	wrapped_bitmap_free(set->bitmap);
}

void bitset_bag_free(bitset_bag_t *b)
{
	// we won't free bitsets[] since some of it is already used by itemtree and the others are already freed when creating tree
	free(b);
}