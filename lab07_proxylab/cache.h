#include "csapp.h"

#ifndef __CACHE_H__
#define __CACHE_H__
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NCACHE 50 

typedef struct 
{
    char host[MAXLINE];
    char uri[MAXLINE];
    size_t state;
    int lru_cnt;
} Cache;

/*
Cache cache[NCACHE];
int cache_cnt;
char pool[NCACHE][MAX_OBJECT_SIZE];
int readcnt;
sem_t mutex, w;
size_t total_cache;
*/
void cache_init(Cache *);
void cache_update(Cache *);
void cache_put(Cache *, int, char *, char *, size_t);
int cache_evict(Cache *);
int cache_isin(Cache *, char *, char *);
int cache_find(Cache *);
#endif