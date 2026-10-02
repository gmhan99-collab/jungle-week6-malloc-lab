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
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double Word size (bytes) */
#define CHUNKSIZE (1<<12)   /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | alloc)

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & -0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp) - DSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp; // 현재 힙 영역의 시작을 가리킴

static char *explicit_listp; // 명시적 가용 리스트 시작 주소

#define PRED(bp) (*(char *)(bp))  // predecessor 주소
#define SUCC(bp) (*(char *)(bp + WSIZE)) // successor 주소
/****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* function prototype */

static void *extend_heap(size_t words); // 힙 확장
static void *coalesce(void *bp); // 가용 공간 병합
static void *find_fit(size_t asize); // first_fit으로 구현되어 있음
static void place(void *bp, size_t asize); // find_fit으로 찾은 메모리에 할당.
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* 패딩용 더미 워드 */

    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* 프롤로그 헤더, 4바이트, 할당된 상태 = 1 */

    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* 프롤로그 푸터, 4바이트, 할당된 상태 = 1 */

    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* 에필로그 헤더 , 크기 0, 할당된 상태 = 1 힙 끝을 표시함 */

    heap_listp += (2*WSIZE); /* heap_listp를 프롤로그 푸터 바로 다음으로 이동 */

    
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)  /* free block 만들기 위해 힙 확장 */
        return -1;
    
    // if ((explicit_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    //     return -1;
    
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Alignment 유지를 위한 짝수 워드 수만큼 크기 조정 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* size만큼 heap 확장 */
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); /* 새로 만든 블록의 헤더 */
    PUT(FTRP(bp), PACK(size, 0)); /* 새로 만든 블록의 푸터 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 새로운 에필로그 헤더, 크기 0, 할당된 상태 = 1 */

    return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
    //     return NULL;
    // else
    // {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
    size_t asize; // 조정된 블록 사이즈
    size_t extendsize; // 맞는 크기가 없을 때 확장 할 힙의 크기
    char *bp;

    // 0바이트 요청 무시
    if (size == 0)
        return NULL;

    // 오버헤드와 정렬 고려해서 블록 사이즈 조정
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        // ( 요청 크기 + 헤더/푸터 크기를 더블워드 단위로 올림 ) <<< 요청 크기 별 할당해야 하는 사이즈 계산식.
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // 알맞은 free 블록 찾기
    if ((bp = find_fit(asize)) != NULL) 
    {
        place(bp, asize);
        return bp;
    }

    // 적당한 블록이 없으면 힙 확장 후 재할당
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) // 힙 확장 실패
        return NULL;
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    for(char *bp = heap_listp;  GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            printf("fit found!\n");
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    if (size - asize >= 2 * DSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); /* 이전 블록의 푸터를 보고 할당 여부 확인 */
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); /* 다음 블록의 헤더를 보고 할당 여부 확인 */
    size_t size = GET_SIZE(HDRP(bp));  /* 현재 블록의 크기 */

    // Case 1 : 이전, 다음 블록 모두 할당됨
    if (prev_alloc && next_alloc)
        return bp;

    // Case 2 : 이전 블록만 할당된 상태
    else if (prev_alloc && !next_alloc) // 헤더, 푸터 그대로 현재 포인터 기준으로 할당하면 됨
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // Case 3 : 다음 블록만 할당된 상태
    else if (!prev_alloc && next_alloc) // 푸터는 현재 포인터 기준, 헤더는 이전 블록의 포인터 기준으로 할당
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    // Case 4 : 이전과 다음 블록 모두 할당되지 않음(free)
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전 블록의 헤더 ~ 다음 블록의 푸터가 새로운 크기임
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}