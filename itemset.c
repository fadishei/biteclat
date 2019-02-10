#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "itemset.h"


#define IS_NEWLINE(c)	((c)=='\n' || (c)=='\r')
#define IS_SEP(c)		((c)==' ' || (c)==',' || c=='\t')
#define IS_NUM(c)		((c)>='0' && (c)<='9')
#define IS_VALID(c)		(IS_NEWLINE(c) || IS_SEP(c) || IS_NUM(c))

itemset_bag_t *itemset_bag_create(char *path, double frac)
{
	long i, j;
	FILE *fp;
	char *buf;
	itemset_bag_t *bag;
	
	bag = (itemset_bag_t *)malloc(sizeof(itemset_bag_t));
	if (!bag)
		goto e1;
	
	fp = fopen(path, "rb");
	if (!fp)
		goto e2;
	
	long size; // length of input in bytes
	fseek(fp , 0L , SEEK_END);
	size = ftell(fp);
	rewind(fp);
	
	buf = malloc(size+1);
	if(!buf) 
		goto e3;
	
	if(fread(buf, size, 1, fp) != 1)
		goto e4;
	if (buf[size-1] != '\n')
	{
		buf[size] = '\n';
		size++;
	}
	
	long ntran; // numebr of transactions
	long l; // line number
	for (l=0, i=0, ntran=0; i<size;)
	{
		while (!IS_NEWLINE(buf[i]) && i<size)
		{
			if (!IS_VALID(buf[i]))
			{
				printf("invalid character %02x at line %ld\n", buf[i], l);
				goto e4;
			}
			i++;
		}
		ntran++;
		while (IS_NEWLINE(buf[i]) && i<size)
		{
			if (!IS_VALID(buf[i]))
			{
				printf("invalid character %02x at line %ld\n", buf[i], l);
				goto e4;
			}
			i++;
			l++;
		}
	}
	long nmax = (long)round(frac*ntran);
	
	bag->itemsets = (itemset_t *)malloc((nmax)*sizeof(itemset_t));
	if (!bag->itemsets)
		goto e4;
	bag->len = nmax;
	bag->item_max = 0;
	
	long start, end; // start and end of current line
	int nitem;
	for (i=0, start=0, end=0, ntran=0; ntran<nmax && i<size;)
	{
		while (!IS_NEWLINE(buf[i]) && i<size)
			i++;
		end = i;
		
		for (j=start, nitem=0; j<end;)
		{
			while (!IS_SEP(buf[j]) && j<end)
				j++;
			nitem++;
			while (IS_SEP(buf[j]) && j<size)
				j++;
		}
		bag->itemsets[ntran].items = (int*)malloc((nitem)*sizeof(int));
		if (!bag->itemsets[ntran].items)
			goto e5;		
		bag->itemsets[ntran].len = nitem;
		
		long istart, iend; // start and end of current item
		for (j=start, nitem=0, istart=start, iend=start; j<end;)
		{
			while (!IS_SEP(buf[j]) && j<end)
				j++;
			iend = j;
			char tmp = buf[end];
			buf[iend] = '\0';
			int item = atoi(buf+istart);
			bag->itemsets[ntran].items[nitem] = item;
			if (item>bag->item_max)
				bag->item_max = item;
			buf[iend] = tmp;
			nitem++;
			while (IS_SEP(buf[j]) && j<size)
				j++;
			istart = j;
		}
		ntran++;
		while (IS_NEWLINE(buf[i]) && i<size)
			i++;
		start = i;
	}
	fclose(fp);
	free(buf);
	return bag;

e5:
	itemset_bag_free(bag);
e4:
	free(buf);
e3:
	fclose(fp);
e2:
	free(bag);
e1:	
	return NULL;
	

}

void itemset_free(itemset_t *itemset)
{
	free(itemset->items);
}

void itemset_bag_free(itemset_bag_t *bag)
{
	long i;
	for (i=0; i<bag->len; i++)
		itemset_free(bag->itemsets+i);
	free(bag->itemsets);
	free(bag);
}

