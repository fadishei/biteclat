#ifndef STATS_H
#define STATS_H

#include <stdio.h>

int stat_init();
void stat_head();
void stat_start();
void stat_stop();
void stat_log(FILE *f);
void stat_finish();

#endif
