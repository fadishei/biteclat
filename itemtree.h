#ifndef ITEMTREE_H
#define ITEMTREE_H

#include "bitset.h"

typedef struct itemnode
{
	int item;
	bitset_t *bitset;
	//long count;
	struct itemnode *right;
	struct itemnode *down;
	struct itemnode *up;
} itemnode_t;

void itemtree_insert_down(itemnode_t *parent, itemnode_t *child);
itemnode_t *itemtree_create(bitset_bag_t *bag, long minsup);
void itemtree_print(itemnode_t *root);
int itemtree_count(itemnode_t *root);
int itemtree_count_maximal(itemnode_t *root);
long itemtree_len_sum(itemnode_t *root);
long itemtree_maximal_len_sum(itemnode_t *root);
void itemtree_free(itemnode_t *root);

#endif