#include "csapp.h"
#include "cache.h"

extern size_t total_cache;

void cache_init(Cache *cache)
{
   int i;
   for (i = 0; i < NCACHE; ++i)
   {
        cache[i].lru_cnt = cache[i].state = 0; 
   } 
}

void cache_update(Cache *cache)
{
    int i;
    for (i = 0; i < NCACHE; ++i)
        if (cache[i].state)
        {
            cache[i].lru_cnt++;
        } 
}

int cache_evict(Cache *cache)
{
    int i, max = 0, idx = -1;
    for (i = 0; i < NCACHE; ++i)
        if (cache[i].lru_cnt > max)
        {
            idx = i;
            max = cache[i].lru_cnt;
        }

    if (idx != -1) 
    {
        total_cache -= cache[idx].state;
        cache[idx].state = 0;
        cache[idx].lru_cnt = 0;
    }

    return idx;
}

void cache_put(Cache *cache, int idx, char *host, char *uri, size_t size)
{
    strcpy(cache[idx].host, host);
    strcpy(cache[idx].uri, uri);
    cache[idx].state = size;
    cache[idx].lru_cnt = 0;
}

int cache_isin(Cache *cache, char *host, char *uri)
{
    int i;
    for (i = 0; i < NCACHE; ++i)
        if (cache[i].state)
        {
            if (!strcmp(cache[i].host, host) &&
                !strcmp(cache[i].uri, uri))
                return i;
        }

    return -1;
}

int cache_find(Cache *cache)
{
    int i;
    for (i = 0; i < NCACHE; ++i)
        if (!cache[i].state) return i;

    return -1;
}