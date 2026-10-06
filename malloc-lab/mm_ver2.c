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

/* 매크로들은 mm.h파일에 위치해 있음 */

static char *heap_listp; // 현재 힙 영역의 시작을 가리킴


#define GET_PRED(bp) (*(char *)(bp))  // predecessor 주소
#define GET_SUCC(bp) (*(char *)(bp + WSIZE)) // successor 주소
/****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

static void *explicit_listp = NULL; // 명시적 가용 리스트 시작 주소
/* function prototype */

static void *extend_heap(size_t words); // 힙 확장
static void *coalesce(void *bp); // 가용 공간 병합
static void *find_fit(size_t asize); // first_fit으로 구현되어 있음
static void place(void *bp, size_t asize); // find_fit으로 찾은 메모리에 할당.
static void push(char *bp);
static void pop(char *bp);
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

#define LIST_COUNT 16
static void *seg_free_list[LIST_COUNT];
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    printf("=== 새 테스트 실행 ===\n");
    explicit_listp = NULL;

    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0); /* 패딩용 더미 워드 */

    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* 프롤로그 헤더, 4바이트, 할당된 상태 = 1 */

    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* 프롤로그 푸터, 4바이트, 할당된 상태 = 1 */

    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* 에필로그 헤더 , 크기 0, 할당된 상태 = 1 힙 끝을 표시함 */

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)    
        return -1;

    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Alignment 유지를 위한 짝수 워드 수만큼 크기 조정 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // 

    /* size만큼 heap 확장 */
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); /* 새로 만든 블록의 헤더 */
    PUT(FTRP(bp), PACK(size, 0)); /* 새로 만든 블록의 푸터 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 새로운 에필로그 헤더, 크기 0, 할당된 상태 = 1 */
    // printf("EXTENDED!\n");
    push(bp);

    return bp;
    // coalesce(bp);
}

static void push(char *bp)
{
    if (explicit_listp == NULL)
    {
        // explicit_listp = (char *)bp;
        // *bp = (char *)NULL; // 이전 연결( top에있으니 NULL )
        // *(bp + DSIZE) = (char *)NULL; // 다음 연결 ( 혼자 있으니 NULL )
        FREE_PUT_NEXT(bp, NULL);
        FREE_PUT_PREV(bp, NULL);
        explicit_listp  = bp;
        // printf("NEW EXPL LIST!\n");
    }
    else
    {
    //    *bp = (char *)NULL; // 이전 연결 (top에 있으니 NULL)
    //     *(bp + 1) = explicit_listp; // 다음 블록 포인터(payload 시작) 가르킴
    //     explicit_listp = (char *)bp;
        FREE_PUT_PREV(bp, NULL);
        FREE_PUT_NEXT(bp, explicit_listp);
        FREE_PUT_PREV(explicit_listp, bp);
        explicit_listp = bp;
        // printf("Added to EXPL LIST!\n");
    }
}
static void pop(char *bp)
{        
    // explicit_listp = (char *)(*(explicit_listp + DSIZE));
    // *bp = (char *)NULL; // pop했으니 다 떼기 (덮어씌워질거지만 일단 처리해보고)
    // *(bp + DSIZE) = (char *)NULL; // pop했으니 다 떼기
    void *prev = FREE_PREV_BLKP(bp);
    void *next = FREE_NEXT_BLKP(bp);

    if (prev != NULL)
        FREE_PUT_NEXT(prev, next);
    else
        explicit_listp = next;

    if (next != NULL)
        FREE_PUT_PREV(next, prev);
    // 앞과 뒤 이어주기
    // *((char *)(*bp)) = *((char *)(*bp) + DSIZE);
    // *((char *)(*(bp + DSIZE))) = *((char *)(*bp));
    FREE_PUT_PREV(bp, NULL);
    FREE_PUT_NEXT(bp, NULL);
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
    if (size <= 2 * DSIZE)
        asize = 3 * DSIZE;
    else
        // ( 요청 크기 + 헤더/푸터 크기를 더블워드 단위로 올림 ) <<< 요청 크기 별 할당해야 하는 사이즈 계산식.
        // asize = DSIZE * ((size + (3*DSIZE) + (DSIZE - 1)) / DSIZE); // 현재 최소 24바이트
        asize = MAX(3 * DSIZE , ALIGN(size + 2 * WSIZE));

    // 알맞은 free 블록 찾기
    if ((bp = find_fit(asize)) != NULL) 
    {
        // printf("FOUND FIT AND MALLOC SUCCESS!\n");
        place(bp, asize);
        // printf("bp = %p, aligned = %d\n", bp, ((uintptr_t)bp % 8) == 0);
        return bp;
    }

    // 적당한 블록이 없으면 힙 확장 후 재할당
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) // 힙 확장 실패
        return NULL;
    // printf("MALLOC SUCCESS AFTER EXTENSION\n");
    // if((bp == find_fit(asize)) == NULL)
    //     place(bp, asize);
    place(find_fit(asize), asize);
    // printf("bp = %p, aligned = %d\n", bp, ((uintptr_t)bp % 8) == 0);
    
    return bp;
}

static void *find_fit(size_t asize)
{
    void *bp = explicit_listp;
    while (bp != NULL)
    {
        // printf("bp = %p\n", bp);
        // printf("header = %u\n", GET(HDRP(bp)));
        // printf("size = %u\n", GET_SIZE(HDRP(bp)));
        // printf("prev = %p\n", FREE_PREV_BLKP(bp));
        // printf("next = %p\n", FREE_NEXT_BLKP(bp));
        if (GET_SIZE(HDRP(bp)) >= asize)
        {
            // printf("fit found!\n");
            pop(bp);
            return bp;
        }

        bp = FREE_NEXT_BLKP(bp);
    }

    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    if (size - asize >= 3 * DSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        // printf("extend bp = %p, mod8 = %lu\n",
    //    bp, (uintptr_t)bp % 8);
        bp = NEXT_BLKP(bp);
        // printf("extend bp = %p, mod8 = %lu\n",
    //    bp, (uintptr_t)bp % 8);
        PUT(HDRP(bp), PACK(size - asize, 0));
        PUT(FTRP(bp), PACK(size - asize, 0));
        push(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
    }
    printf("Place Success!\n");
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(ptr == NULL)
        return;
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    push(ptr);
    // printf("FREED!\n");
    // coalesce(ptr);
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