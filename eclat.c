#include <stdlib.h>
#include "eclat.h"

void eclat_rec(itemnode_t *prefix_end, itemnode_t *item_start, long minsup)
{
	itemnode_t *node;
	
	for (node=item_start; node!=NULL; node=node->right)
	{
		wrapped_bitmap_t *r = wrapped_bitmap_and(prefix_end->bitset->bitmap, node->bitset->bitmap);
		long freq = wrapped_bitmap_get_cardinality(r);
		if (freq < minsup)
		{
			wrapped_bitmap_free(r);
			continue;
		}
		
		itemnode_t *n = (itemnode_t *)malloc(sizeof(itemnode_t));
		n->item = node->item;
		n->bitset = (bitset_t *)malloc(sizeof(bitset_t));
		n->bitset->bitmap = r;
		n->bitset->card = freq; // need this?
		//n->count = freq;
		n->down = NULL;
		itemtree_insert_down(prefix_end, n);
		
		eclat_rec(n, node->right, minsup);
	}
}

void eclat(itemnode_t *root, long minsup)
{
	itemnode_t *node;
	
	for (node=root; node!=NULL; node=node->right)
		eclat_rec(node, node->right, minsup);
}
