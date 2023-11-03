#include "cachelab.h"
#include "unistd.h"
#include "getopt.h"
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h"
#include "string.h"

struct Cache
{
    unsigned s;
    unsigned E;
    unsigned b;
    unsigned t;
} cache;

struct line
{
    int valid;
    unsigned long tag;
    int LRU_cnt;
};

void init_cache(void);
int quick_power(int, int );
void load(unsigned long, int);
void store(unsigned long, int);
void modify(unsigned long, int);
void set_and_update_count(unsigned, unsigned, unsigned long);
void find(unsigned long ,int);

unsigned hit_cnt, miss_cnt, evict_cnt;
unsigned S, B;
struct line** cache_line;
unsigned long setmask, tagmask;

int main(int argc, char** argv)
{
    int opt;
    FILE* p_file;

    while (-1 != (opt = getopt(argc, argv, "vs:E:b:t:")))
    {
        switch (opt)
        {
            case 'v':
                // print_verbose();
                break;
            case 's':
                cache.s = atoi(optarg);
                break;
            case 'E':
                cache.E = atoi(optarg);
                break;
            case 'b':
                cache.b = atoi(optarg);
                break;
            case 't':
                if (NULL == (p_file = fopen(optarg, "r")))
                {
                    printf("open file failed!\n");
                    exit(1);
                }
                break;
            default:
                printf("argument error!\n");
                exit(1);
        }
    }

    init_cache();

    unsigned long address;
    unsigned d_size;
    char ins; 
    while (-1 != (fscanf(p_file, " %c %lx, %d", &ins, &address, &d_size)))
    {
        if (ins != 'I') printf("%c %lx,%u ", ins, address, d_size);
        switch (ins)
        {
            case 'I': break;
            case 'L':
                load(address, d_size);
                break;
            case 'S':
                store(address, d_size);
                break;
            case 'M':
                modify(address, d_size);
                break;
            default:
                printf("unknown instruction!\n");
                exit(1);
        }
    }

    fclose(p_file);
    
    for (int i = 0; i < S; ++i) free(cache_line[i]);
    free(cache_line);

    printf("hits:%d misses:%d evictions:%d\n", hit_cnt, miss_cnt, evict_cnt);
    printSummary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}

int quick_power(int a, int b)
{
    int res = 1;
    while (b)
    {
        if (b & 1) res = res * a;

        a = a * a;

        b >>= 1;
    }

    return res;
}

void init_cache(void)
{
    cache.t = 64 - cache.s - cache.b;
    S = quick_power(2, cache.s), B = quick_power(2, cache.b);
    cache_line = (struct line**)malloc(sizeof(struct line*) * S);
    for (int i = 0; i < S; ++i) cache_line[i] = (struct line*)malloc(sizeof(struct line) * cache.E);
    for (int i = 0; i < S; ++i) memset(cache_line[i], 0, sizeof(struct line) * cache.E);

    for (int i = cache.b; i < cache.b + cache.s; ++i) setmask |= (1 << i);
    for (int i = cache.b + cache.s; i < 64; ++i) tagmask |= (1 << i);
}

void find(unsigned long address, int size)
{
    unsigned long set_idx = (address & setmask) >> cache.b;
    unsigned long tag = (address & tagmask) >> (cache.s + cache.b);
    int t = -1;

    for (int j = 0; j < cache.E; ++j)
    {
        if (cache_line[set_idx][j].valid == 1)
        {
            if (cache_line[set_idx][j].tag == tag) // hit
            {
                printf("hit ");
                ++hit_cnt;
                set_and_update_count(set_idx, j, tag);
                return;
            }
        }
        else t = j;
    }
        
    // miss
    printf("miss ");
    ++miss_cnt;
    if (t != -1) // The line is still not full
    {
        set_and_update_count(set_idx, t, tag);
    }
    else // have to evict a line
    {
        printf("eviction ");
        ++evict_cnt;
        int max = 0, max_idx = 0;
        for (int j = 0; j < cache.E; ++j)
            if (cache_line[set_idx][j].LRU_cnt > max)
            {
                max_idx = j;
                max = cache_line[set_idx][j].LRU_cnt;
            }

        set_and_update_count(set_idx, max_idx, tag);
    }
}

void load(unsigned long address, int size)
{
    find(address, size);
    printf("\n");
}

void store(unsigned long address, int size)
{
    find(address, size);
    printf("\n");
}

void modify(unsigned long address, int size)
{
    find(address, size);
    find(address, size);
    printf("\n");
}

void set_and_update_count(unsigned set_idx, unsigned line_num, unsigned long tag)
{
    cache_line[set_idx][line_num].LRU_cnt = -1;
    cache_line[set_idx][line_num].tag = tag;
    cache_line[set_idx][line_num].valid = 1;

    for (int j = 0; j < cache.E; ++j)
        ++cache_line[set_idx][j].LRU_cnt;
}