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
    "XJTU ICS",
    /* First member's full name */
    "Derrick Liu",
    /* First member's email address */
    "lzy1102@stu.xjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define WORD_SIZE (sizeof(unsigned int)) //4 B
#define PTR_SIZE (sizeof(void *)) //8 B

#define READ(PTR) (*(unsigned int *)(PTR))
#define WRITE(PTR, VALUE) ((*(unsigned int *)(PTR)) = (VALUE)) //WRITE IN 4B

#define PACK(SIZE, IS_ALLOC) ((SIZE) | (IS_ALLOC))

#define GET_SIZE(PTR) (unsigned int)((READ(PTR) >> 3) << 3)
#define IS_ALLOC(PTR) (READ(PTR) & (unsigned int)1)

#define HEAD_PTR(PTR) ((void *)(PTR) - WORD_SIZE)
#define TAIL_PTR(PTR) ((void *)(PTR) + GET_SIZE(HEAD_PTR(PTR)) - WORD_SIZE * 2) //ptr +size先到达下一个块的数据位开头，往前两个wordsize就是上一个的tail

#define NEXT_BLOCK(PTR) ((void *)(PTR) + GET_SIZE(HEAD_PTR(PTR)))
#define PREV_BLOCK(PTR) ((void *)(PTR) - GET_SIZE((void *)(PTR) - WORD_SIZE * 2))

#define NEXT_AVAI_ADDR(PTR) ((void **)PTR)
#define PREV_AVAI_ADDR(PTR) ((void **)(NEXT_AVAI_ADDR(PTR) + 1))

#define READ_NEXT_AVAI_PTR(PTR) (*NEXT_AVAI_ADDR(PTR))
#define READ_PREV_AVAI_PTR(PTR) (*PREV_AVAI_ADDR(PTR))

#define PAGE_SIZE (1 << 12) //4096
#define WRITE_PTR(PTR, ADDR) (*(void **)(PTR) = ADDR)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
// /* 
//  * mm_init - initialize the malloc package.
//  */
// int mm_init(void)
// {
//     return 0;
// }

// /* 
//  * mm_malloc - Allocate a block by incrementing the brk pointer.
//  *     Always allocate a block whose size is a multiple of the alignment.
//  */
// void *mm_malloc(size_t size)
// {
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);
//     if (p == (void *)-1)
// 	return NULL;
//     else {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// }

// /*
//  * mm_free - Freeing a block does nothing.
//  */
// void mm_free(void *ptr)
// {
// }

// /*
//  * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
//  */
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = ptr;
//     void *newptr;
//     size_t copySize;    
//     newptr = mm_malloc(size);
//     if (newptr == NULL)
//       return NULL;
//     copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//     if (size < copySize)
//       copySize = size;
//     memcpy(newptr, oldptr, copySize);
//     mm_free(oldptr);
//     return newptr;
// }
void *HeapList = NULL;

/**
 * remove(ptr) & insert(ptr)
 */

void Remove(void *ptr){
    if (IS_ALLOC(HEAD_PTR(ptr)))
    {
        return;
    }
    else
    {
        void *Prev = READ_PREV_AVAI_PTR(ptr);
        void *Next = READ_NEXT_AVAI_PTR(ptr);
        /*
        Prev NULL, Next NULL;
        Prev NULL, Next not NULL;
        Prev not NULL, Next NULL;
        Prev & Next not NULL.
        */
        if (Prev == NULL && Next == NULL) {
            return;
        }
        else if (Prev == NULL && Next != NULL) {
            WRITE_PTR((void **)Next + 1, NULL);
            return;
        }
        else if (Prev != NULL && Next == NULL) {
            WRITE_PTR(Prev, NULL);
            return;
        }
        else {
            WRITE_PTR((void **)Next + 1, Prev);
            WRITE_PTR(Prev, Next);
            return;
        }
    }
    
}

void Insert(void *ptr) {
    if (IS_ALLOC(HEAD_PTR(ptr))) {
        return;  // 如果块已分配，直接返回
    }

    void *Prev = READ_PREV_AVAI_PTR(ptr);
    void *Next = READ_NEXT_AVAI_PTR(ptr);


    // if (Prev == NULL && Next == NULL) {
    //     void **p;
    //     for (p = (void **)HeapList + 1; p < (void **)ptr ;p = (void **)GOTO_NEXT_BLOCK(p)) {
    //         WRITE_PTR(p, ptr); 
    //     }
    //     void **q;
    //     for (q = (void **)ptr + 1; GET_SIZE(HEAD_PTR(q)) > 0; q = (void **)GOTO_NEXT_BLOCK(q)) {
    //         WRITE_PTR((void **)q + 1, ptr); // 将后续块的 prev 指针设置为 NULL
    //     }
    // }
    if (Prev == NULL) {
        void **p;
        for (p = (void **)HeapList + 1; p < (void **)ptr ;p = (void **)NEXT_BLOCK(p)) {
            WRITE_PTR(p, ptr); 
        }
    }
    else{
        WRITE_PTR(Prev, ptr); // 将前一个块的 next 指针设置为当前块
    }
    if (Next == NULL) {
        void **q;
        for (q = (void **)ptr + 1; GET_SIZE(HEAD_PTR(q)) > 0; q = (void **)NEXT_BLOCK(q)) {
            WRITE_PTR((void **)q + 1, ptr); // 将后续块的 prev 指针设置为 PTR
        }
    }
    else{
        WRITE_PTR((void **)Next + 1, ptr); // 将后续块的 prev 指针设置为当前块
    }
}

void *Merge(void *Ptr) {
    // 获取当前块、前一个块和后一个块的状态
    void *Prev = PREV_BLOCK(Ptr);
    void *Next = NEXT_BLOCK(Ptr);
    
    unsigned int PrevAlloc = IS_ALLOC(HEAD_PTR(Prev)); 
    unsigned int NextAlloc = IS_ALLOC(HEAD_PTR(Next));
    unsigned int Size = GET_SIZE(HEAD_PTR(Ptr));
    // Case 1: 前后都被占用
    if(PrevAlloc && NextAlloc) {
        WRITE(HEAD_PTR(Ptr), PACK(Size, 0));
        WRITE(TAIL_PTR(Ptr), PACK(Size, 0));
        return Ptr;
    }
    
    // Case 2: 前占用,后空闲
    if(PrevAlloc && !NextAlloc) {
        Size += GET_SIZE(HEAD_PTR(Next));
        Remove(Next);
        WRITE(HEAD_PTR(Ptr), PACK(Size, 0));
        WRITE(TAIL_PTR(Next), PACK(Size, 0));
        Insert(Ptr);
        return Ptr;
    }
    
    // Case 3: 前空闲,后占用
    if(!PrevAlloc && NextAlloc) {
        Size += GET_SIZE(HEAD_PTR(Prev));
        Remove(Prev);
        WRITE(HEAD_PTR(Prev), PACK(Size, 0));
        WRITE(TAIL_PTR(Ptr), PACK(Size, 0));
        Insert(Prev);
        return Prev;
    }
    
    // Case 4: 前后都空闲
    if(!PrevAlloc && !NextAlloc) {
        Size += GET_SIZE(HEAD_PTR(Prev)) + GET_SIZE(HEAD_PTR(Next));
        Remove(Prev);
        Remove(Next);
        WRITE(HEAD_PTR(Prev), PACK(Size, 0));
        WRITE(TAIL_PTR(Next), PACK(Size, 0));
        Insert(Prev);
        return Prev;
    }

    return Ptr;
}

void Place(void *Ptr, unsigned int Size) {
    unsigned int BlockSize = GET_SIZE(HEAD_PTR(Ptr));
    Remove(Ptr);
    
    // 如果分割后剩余空间不足以放置一个新的块(至少24字节),则分配整个块
    if(BlockSize - Size < 24) {
        WRITE(HEAD_PTR(Ptr), PACK(BlockSize, 1));
        WRITE(TAIL_PTR(Ptr), PACK(BlockSize, 1));
        return;
    }
    
    WRITE(HEAD_PTR(Ptr), PACK(Size, 1));
    WRITE(TAIL_PTR(Ptr), PACK(Size, 1));
    
    void *Next = NEXT_BLOCK(Ptr);
    WRITE(HEAD_PTR(Next), PACK(BlockSize - Size, 0));
    WRITE(TAIL_PTR(Next), PACK(BlockSize - Size, 0));
    Insert(Next);
}


void *FirstFit(size_t Size) {
    void *Ptr = READ_NEXT_AVAI_PTR(HeapList + WORD_SIZE * 2);
    
    // 遍历整个堆
    while(Ptr != NULL) {
        if(GET_SIZE(HEAD_PTR(Ptr)) >= Size) {
            return Ptr;
        }
        Ptr = READ_NEXT_AVAI_PTR(Ptr);
    }
    
    return NULL;
}

int mm_init() {
    HeapList = mem_sbrk(WORD_SIZE << 3); //32B
    if (HeapList == (void *)-1) return -1;
    // Fill in metadata as initial space
    WRITE(HeapList, 0); // prologue: 0000 3001 NULL(8) ADDR(8) 3001 | tail: 0001
    // Prologue block
    WRITE(HeapList + WORD_SIZE, PACK(24, 1)); // Prologue's header
    WRITE_PTR(HeapList + WORD_SIZE * 2, NULL); // Prologue's NEXT pointer
    WRITE_PTR(HeapList + PTR_SIZE + WORD_SIZE * 2, NULL);
    WRITE(HeapList + WORD_SIZE * 2 + PTR_SIZE * 2, PACK(24, 1));
    WRITE(HeapList + PTR_SIZE * 2 + WORD_SIZE * 3, PACK(0, 1));
    return 0;
}

//WAIT TO BE MODIFIED
void *mm_malloc(size_t size) {
    // If size equals zero, which means we don't need to execute malloc
    if (size == 0) return NULL;
    // Add header size and tailer size to block size
    size += (WORD_SIZE << 1);
    // Round up size to mutiple of 8
    if ((size & (unsigned int)7) > 0) size += (1 << 3) - (size & 7);
    // We call first fit function to find a space with size greater than argument 'size'
    void *Ptr = FirstFit(size);
    // If first fit function return NULL, which means there's no suitable space.
    // Else we find it. The all things to do is to place it.
    if (Ptr != NULL) {
        Place(Ptr, size);
        Remove(Ptr);
        void *NewAvailable = NEXT_BLOCK(Ptr);
        Insert(NewAvailable);
        return Ptr;
    }
    // We call sbrk to extend heap size
    unsigned int SbrkSize = MAX(size, PAGE_SIZE);
    void *NewPtr = mem_sbrk(SbrkSize);
    if (NewPtr == (void *)-1) return NULL;
    // Write metadata in newly requested space
    WRITE(NewPtr - WORD_SIZE, PACK(SbrkSize, 0));
    WRITE(mem_heap_hi() - 3 - WORD_SIZE, PACK(SbrkSize, 0));
    WRITE(mem_heap_hi() - 3, PACK(0, 1));
    // Execute function merge to merge new space and free block in front of it
    NewPtr = Merge(NewPtr);
    // Execute function place to split the free block to 1/2 parts
    Place(NewPtr, size);
    Remove(NewPtr);
    void *NewAvailable = NEXT_BLOCK(NewPtr);
    Insert(NewAvailable);
    return NewPtr;
}

//WAIT TO BE MODIFIED
void mm_free(void *ptr) {
    // We just fill in the header and tailer with PACK(Size, 0)
    void *Header = HEAD_PTR(ptr), *Tail = TAIL_PTR(ptr);
    unsigned int Size = GET_SIZE(Header);
    WRITE(Header, PACK(Size, 0));
    WRITE(Tail, PACK(Size, 0));
    // Then merge it with adjacent free blocks
    Merge(ptr);
    // the block turns into available one, needing Insert(ptr)
    // Insert(ptr);  
}

//WAIT TO BE MODIFIED
void *mm_realloc(void *ptr, size_t size) {
    // We get block's original size
    unsigned int BlkSize = GET_SIZE(HEAD_PTR(ptr));
    // Round up size to mutiple of 8
    if ((size & (unsigned int)7) > 0) size += (1 << 3) - (size & 7); //把size补成8的倍数
    // If original size is greater than requested size, we don't do any.
    if (BlkSize >= size + WORD_SIZE * 2) return ptr;
    // Else, we call malloc to get a new space for it.
    void *NewPtr = mm_malloc(size);
    if (NewPtr == NULL) return NULL;
    // Move the data to new space
    memmove(NewPtr, ptr, size);
    // Free old block
    mm_free(ptr);
    return NewPtr;
}
