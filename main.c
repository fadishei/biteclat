#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>
#include "itemset.h"
#include "itemtree.h"
#include "eclat.h"
#include "stats.h"
#ifdef MEMPROF
#include <gperftools/heap-profiler.h>
#endif

int verbosity = 0;

void print_help(FILE *fp)
{
	fprintf(fp, "usage: eclat [options]\n");
	fprintf(fp, "options:\n");
	fprintf(fp, "-d <dataset>  dataset file. csv of numbers. one transaction per line\n");
	fprintf(fp, "-f <frac>     fraction of transactions to process from start. default 1.0\n");
	fprintf(fp, "-h            print help\n");
	fprintf(fp, "-H            print header\n");
	fprintf(fp, "-m <sup>      minimum support. default 0.1\n");
	fprintf(fp, "-p            print frequent patterns\n");
	fprintf(fp, "-s            print stats\n");
	fprintf(fp, "-v            be verbose\n");
}

void verbose(char *fmt, ...)
{
	if (!verbosity)
		return;

	va_list args;
   	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fflush(stderr);
	va_end(args);	
}

int main(int argc, char *argv[])
{
	int c;
	char *infile = NULL;
	double minsupf = 0.1;
	long minsup;
	int printhd = 0, printfp = 0, printst = 0;
	double frac = 1.0;
	
	while ((c=getopt(argc, argv, "d:f:hHm:psv")) != -1)
	{
		switch (c)
		{
			case 'd':
				infile = optarg;
				break;
			case 'h':
				print_help(stdout);
				exit(0);
			case 'H':
				printhd = 1;
				break;
			case 'm':
				minsupf = atof(optarg);
				if (minsupf<=0)
				{
					fprintf(stderr, "invalid minsup %s\n", optarg);
					exit(1);
				}
				break;
			case 'f':
				frac = atof(optarg);
				if (frac<=0)
				{
					fprintf(stderr, "invalid fraction of transactions %s\n", optarg);
					exit(1);
				}
				break;
			case 'p':
				printfp = 1;
				break;
			case 's':
				printst = 1;
				break;
			case 'v':
				verbosity = 1;
				break;
			default:
				print_help(stderr);
				exit(1);
		}
	}

	if (!printhd && !infile)
	{
		print_help(stderr);
		exit(1);
	}

	stat_init();

	if (printhd)
	{
		stat_head(stdout);
		printf(",count,count_maximal,avg,avg_maximal\n");
	}

	itemnode_t *root;
	if (infile)
	{
		verbose("reading %2.1f%% of input file %s\n", frac*100, infile);
		itemset_bag_t *ibag = itemset_bag_create(infile, frac);
		if (!ibag)
		{
			fprintf(stderr, "can not read infile %s\n", argv[2]);
			exit(1);
		}
		verbose("read %ld transactions\n", ibag->len);
		minsup = (long)(ceil(minsupf*ibag->len));
		verbose("minimum support is %2.1f%% = %ld\n", minsupf*100, minsup);

		if (printst)		
			stat_start();
#ifdef MEMPROF
		HeapProfilerStart("memprof");
#endif

		verbose("creating bitsets\n");
		bitset_bag_t *bbag = bitset_bag_create(ibag);
		itemset_bag_free(ibag);
		verbose("mining bitsets\n");
		root = itemtree_create(bbag, minsup);
		bitset_bag_free(bbag);
		eclat(root, minsup);
		
		if (printst)
			stat_stop();
#ifdef MEMPROF
		HeapProfilerStop();
#endif

		verbose("found frequent itemsets\n");
		if (printfp)
			itemtree_print(root);
		if (printst)
		{
			stat_log(stdout);
			int cnt = itemtree_count(root);
			int mcnt = itemtree_count_maximal(root);
			printf(",%d,%d,%f,%f\n", cnt, mcnt, ((double)itemtree_len_sum(root))/cnt, ((double)itemtree_maximal_len_sum(root))/mcnt);
		}
		itemtree_free(root);
	}

	stat_finish();

	return 0;
}
