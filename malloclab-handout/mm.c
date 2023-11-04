/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "LILIANGJI",
    /* First member's full name */
    "LI LIANGJI",
    /* First member's email address */
    "liliangji@inha.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
/* (x & ~0x7) is equiavalent to [x / 8]*/
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*************************************************************/
/* Below are private global rariables for implicit free list*/
static char *heap_listp; // point to the footer of prologue block
/* Below are macros for implicit free list*/
#define WSIZE 4
#define DSIZE 8
#define CHUNSIZE (1 << 12) // Extend heap by this amount equivalen to PAGE size
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p)) // get the first word to which p points to
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7) // p must point to either header or footer
#define GET_ALLOC(p) (GET(p) & 0x1)
/* bp is a pointer to payload of the current block*/
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
/* get the bp pointer of the next or previous block*/
#define NEXT_BLOCK(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLOCK(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
/*************************************************************/

/***********************************************************************/
/* Here are some additional functions*/
/* coalesce blocks */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLOCK(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLOCK(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) return bp;
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLOCK(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        /*
            Since macro for calculating is based on HDRP
            and the header has been updated, 
            we can make use of FTRP to calculate the footer of the newly coalesced block 
        */
        PUT(FTRP(bp), PACK(size, 0)); 
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLOCK(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLOCK(bp)), PACK(size, 0));
        bp = PREV_BLOCK(bp); /* update bp */
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLOCK(bp))) + GET_SIZE(FTRP(NEXT_BLOCK(bp)));
        bp = PREV_BLOCK(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    return bp;
}
/* extend the heap by words */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* check whether the words is even or not because the words must be a multiple of 8 */
    size = (words % 2) ? ((words + 1) * WSIZE) : (words * WSIZE);
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL; /* allocate a free block of size bytes on the heap  */ 

    PUT(HDRP(bp), PACK(size, 0)); /* set the header of the free block */
    PUT(FTRP(bp), PACK(size, 0)); /* set the footer */
    PUT(HDRP(NEXT_BLOCK(bp)), PACK(0, 1)); /* update epilogue */

    return coalesce(bp);
}
/* find an appropriate block that satisfies the block size */
static void *find_fit(size_t size)
{
    /* first hit */
    for (char *curr = heap_listp; GET_SIZE(HDRP(curr)) != 0; curr = NEXT_BLOCK(curr))
        if (!GET_ALLOC(HDRP(curr)) && (GET_SIZE(HDRP(curr)) >= size))
            return curr;
    
    return NULL; /* failed to find a fit block */
}
/* suppose that we have found an appropriate block to allocate, and split it if possible */
static void place(void* bp, size_t asize)
{
    /* 
        Since both asize and the block size to which bp points to are multiples of 8,
        GET_SIZE(HDRP(bp)) - asize can be divided by 8        
    */
    size_t size = GET_SIZE(HDRP(bp));
    if ((size - asize) >= 16) /* it can be split */
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLOCK(bp); /* base pointer to new split free block */
        PUT(HDRP(bp) , PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
        //coalesce(bp);
    }
    else /* can not be split */
    {
        /* the size of original block can not be changed */
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
}
/***********************************************************************/
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /*
        we need a prologue and a epilogue which are , respectively, 2 words and 1 word.
        This is, total are 3 words (12 bytes).
        Since the heap must be 8-byte aligned, there is a word for padding.
    */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0); /* the firt word is padding */
    PUT(heap_listp + WSIZE, PACK(0x8, 1)); /* initialize the header of prologue */
    PUT(heap_listp + DSIZE, PACK(0x8, 1)); /* initialize the footer of prologue */
    PUT(heap_listp + (3 * WSIZE), PACK(0x0, 1)); /* initialize epilgue */

    heap_listp += DSIZE; /* let heap_listp point to the footer of prologue */

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; /* aligned size */
    size_t extendsize; /* extended size if no fit */
    char *bp = NULL;

    if (size == 0) return NULL;
    
    if (size <= DSIZE) asize = DSIZE * 2; /* allocate a minimum block of 16 bytes */
    else asize = ALIGN(size + DSIZE); /* align the size NOTE: not forget to plus size for header and footer */

    if ((bp = find_fit(asize)) != NULL) place(bp, asize);
    else /* did not find an appropriate block */
    {
        extendsize = MAX(asize, CHUNSIZE);
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
        place(bp, asize);
    }

    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t currsize, asize;
    
    if (ptr == NULL) return mm_malloc(size);
    if (!size)
    {
        mm_free(ptr);
        return NULL;
    }

    currsize = GET_SIZE(HDRP(ptr));
    asize = ALIGN(size + DSIZE);  

    //printf("size = %d, asize = %d, currsize = %d\n", size, asize, currsize);
    if (currsize == asize) return ptr;
    else if (currsize > asize)
    {
        /* 
            we don't need to any allocate new block,
            so not need to copy any memory,
            but we need to check whether the remaining can be split
         */
        //place(ptr, asize);
        //puts("0");
        return ptr;
    }
    else /* In this case, we have to reallocate or coalesce */
    {
        size_t prevalloc, nextalloc;
        size_t prevsize, nextsize;
        char *prevbp, *nextbp;
        prevbp = PREV_BLOCK(ptr), nextbp = NEXT_BLOCK(ptr);
        prevalloc = GET_ALLOC(HDRP(prevbp));
        nextalloc = GET_ALLOC(HDRP(nextbp));
        prevsize = GET_SIZE(HDRP(prevbp));
        nextsize = GET_SIZE(HDRP(nextbp));

        if (!prevalloc && ((prevsize + currsize) >= asize))
        {
            //puts("1");
            /*
            In this case, we can coalesce the current block with the previous one
            */
            //char *i, *j; /* used for copying data */
            PUT(HDRP(prevbp), PACK(prevsize + currsize, 1));
            PUT(FTRP(prevbp), PACK(prevsize + currsize, 1)); 
            /* copy data, NOTE: there are overlapped area, if we used memcpy  */
            //for (i = ptr, j = prevbp; i != FTRP(ptr); ++i, ++j)
            //    *j = *i;
            memcpy(prevbp, ptr, currsize - DSIZE);
            /* check whether the coalesced block can be split */
            //place(prevbp, prevsize + currsize);

            return prevbp;
        }
        else if (!nextalloc && ((nextsize + currsize) >= asize))
        {
            //puts("2");
            /*  
                In this case, we can coalesce the current block with the next one
                NOTE: we don't need to copy any data
            */
             PUT(HDRP(ptr), PACK(nextsize + currsize, 1));
             PUT(FTRP(ptr), PACK(nextsize + currsize, 1));

             return ptr;
        }
        else if ((!prevalloc && !nextalloc) && ((prevsize + currsize + nextsize) >= asize))
        {
            /*
                In this case, we can coalesce the current block with both adjacent blocks
            */
            //char *i, *j;
            //puts("3");
            PUT(HDRP(prevbp), PACK(prevsize + currsize + nextsize, 1));
            PUT(FTRP(prevbp), PACK(prevsize + currsize + nextsize, 1));
            //for (i = ptr, j = prevbp; i != FTRP(ptr); ++i, ++j)
           //     *j = *i;
           memcpy(prevbp, ptr, currsize - DSIZE);
        
            //place(prevbp, prevsize + currsize + nextsize); 
            return prevbp;
        }
        else /* can not coalesce neither previous nor next block */
        {
            //puts("4");
            /* we must allocate a new block such that the specifie size can be allocated */
            newptr = mm_malloc(size);
            memcpy(newptr, oldptr, currsize);
            mm_free(oldptr);
            return newptr;
        }
    }
}
/*
    checker
*/
int mm_checker(void)
{
    printf("This is mm_checker");

    return -1;
}