#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "stats.h"

#define STAT_RAPL_MAX	10
#define STAT_PATH_MAX	1024
#define STAT_COLLECT_INTERVAL	60

char *stat_rapl[STAT_RAPL_MAX+1];
char *stat_rapl_name[STAT_RAPL_MAX+1];
long stat_e1[STAT_RAPL_MAX+1];
long stat_e2[STAT_RAPL_MAX+1];
long stat_emax[STAT_RAPL_MAX+1];
struct timespec stat_t1, stat_t2;
double stat_t, stat_e[STAT_RAPL_MAX+1];
long stat_m;
char fin;
pthread_t stat_collect_thread;
pthread_mutex_t stat_collect_mutex;

long stat_get_vsize()
{
	long s = -1;
	FILE *f = fopen("/proc/self/statm", "r");
	if (!f)
		return -1;
	if (fscanf(f, "%ld", &s)!=1)
		fprintf(stderr, "can not read statm\n");
	fclose (f);
	return s*getpagesize();
}

long stat_get_energy(char *rapl)
{
	long s = -1;
	char p[STAT_PATH_MAX];
 
	snprintf(p, STAT_PATH_MAX, "/sys/class/powercap/%s/energy_uj", rapl);
	FILE *f = fopen(p, "r");
	if (!f)
	{
		fprintf(stderr, "rapl can not open energy_uj\n");
		return -1;
	}
	if (fscanf(f, "%ld", &s)!=1)
		fprintf(stderr, "rapl can not scanf energy_uj\n");
	fclose (f);
	return s;
}

int stat_init()
{
	struct dirent *ent;
	DIR *d = opendir("/sys/class/powercap");
	if (!d) 
		return -1;
	
	int i = 0;
	stat_rapl[i] = NULL;
	while ((ent = readdir(d)) != NULL) 
	{
		if (ent->d_type==DT_LNK && strncmp(ent->d_name, "intel-rapl:", 11)==0)
		{
			char p[STAT_PATH_MAX];
			FILE *f;
			long s;
			
			snprintf(p, STAT_PATH_MAX, "/sys/class/powercap/%s/name", ent->d_name);
			f = fopen(p, "r");
			if (!f)
				continue;
			if (fscanf(f, "%s", p)!=1)
				fprintf(stderr, "rapl can not scanf name\n");
			fclose(f);
			stat_rapl_name[i] = strdup(p);
			stat_rapl[i] = ent->d_name;
			
			snprintf(p, STAT_PATH_MAX, "/sys/class/powercap/%s/max_energy_range_uj", ent->d_name);
			f = fopen(p, "r");
			if (!f)
			{
				fprintf(stderr, "rapl can not open max_energy_range_uj\n");
				continue;
			}
			if (fscanf(f, "%ld", &s)!=1)
				fprintf(stderr, "rapl can not scanf max_energy_range_uj\n");
			fclose(f);
			stat_emax[i] = s;
			
			stat_e[i] = 0.0;
			stat_rapl[++i] = NULL;
		}
	}
	stat_t = 0.0;
	fin = 0;
	pthread_mutex_init(&stat_collect_mutex, NULL);
}

void stat_finish()
{
	int i;
	//pthread_join(stat_collect_thread, NULL);
	for (i=0; stat_rapl[i]!=NULL; i++)
		free(stat_rapl_name[i]);
}

void stat_collect()
{
	int i;
	pthread_mutex_lock(&stat_collect_mutex);
	clock_gettime(CLOCK_REALTIME, &stat_t2);
	stat_t += (stat_t2.tv_sec - stat_t1.tv_sec) + (stat_t2.tv_nsec - stat_t1.tv_nsec)/1000000000.0;
	stat_t1 = stat_t2;

	for (i=0; stat_rapl[i]!=NULL; i++)
		stat_e2[i] = stat_get_energy(stat_rapl[i]);
	for (i=0; stat_rapl[i]!=NULL; i++)
	{
		long diff = (stat_e2[i]-stat_e1[i]);
		if (diff<0)
		{
			diff += stat_emax[i];
			fprintf(stderr, "rapl overflow\n");
		}
		stat_e[i] += diff/1000000.0;
	}
	for (i=0; stat_rapl[i]!=NULL; i++)
		stat_e1[i] = stat_e2[i];
	pthread_mutex_unlock(&stat_collect_mutex);
}

// rapl counters overflow more often than not
void *stat_periodic_collect(void *arg)
{
	while (!fin)
	{
		stat_collect();
		sleep(STAT_COLLECT_INTERVAL);
	}
}

void stat_start()
{
	int i;
	clock_gettime(CLOCK_REALTIME, &stat_t1);
	for (i=0; stat_rapl[i]!=NULL; i++)
		stat_e1[i] = stat_get_energy(stat_rapl[i]);
	pthread_create(&stat_collect_thread, NULL, stat_periodic_collect, NULL);
}

void stat_stop()
{
	fin = 1;
	stat_collect();
	stat_m = stat_get_vsize();
}

void stat_head(FILE *fp)
{
	int i;
	fprintf(fp, "time,memory");
	for (i=0; stat_rapl[i]!=NULL; i++)
		fprintf(fp, ",energy_%s", stat_rapl_name[i]);
}

void stat_log(FILE *fp)
{
	int i;
	fprintf(fp, "%f,%ld", stat_t, stat_m);
	for (i=0; stat_rapl[i]!=0; i++)
		fprintf(fp, ",%f", stat_e[i]);
}
