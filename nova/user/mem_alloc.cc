#include "mem_alloc.h"
#include <stdio.h>

/*
 * Template for 11malloc. If you want to implement it in C++, rename
 * this file to mem_alloc.cc.
 */

static inline void *nbrk(void *address);

#ifdef NOVA

/**********************************/
/* nbrk() implementation for NOVA */
/**********************************/

static inline unsigned syscall2(unsigned w0, unsigned w1)
{
    asm volatile("   mov %%esp, %%ecx    ;"
                 "   mov $1f, %%edx      ;"
                 "   sysenter            ;"
                 "1:                     ;"
                 : "+a"(w0)
                 : "S"(w1)
                 : "ecx", "edx", "memory");
    return w0;
}

static void *nbrk(void *address)
{
    return (void *)syscall2(3, (unsigned)address);
}
#else

/***********************************/
/* nbrk() implementation for Linux */
/***********************************/

#include <unistd.h>

static void *nbrk(void *address)
{
    void *current_brk = sbrk(0);
    if (address != NULL)
    {
        int ret = brk(address);
        if (ret == -1)
            return NULL;
    }
    return current_brk;
}

#endif

#define BLOCKSIZE_MASK 0x3
typedef unsigned long mword;

inline mword get_size(mword *block_start);
inline bool get_present(mword *block_start);
void set_size(mword *block_start, mword size);
void set_present(mword *block_start, bool present);
inline mword *get_next(mword *block_start);
inline mword *get_block_start(mword *block_end_meta);
inline mword *get_previous(mword *block_start);
inline mword *get_block_end(mword *block_start);
bool initial_heap_setup(mword first_block_size);
mword *first_match(mword size);
bool expand_tail(mword block_size);
void split_and_alloc(mword *block_start, mword req_size);
void free_and_merge(mword *block_start);
void merge_blocks(mword *first, mword *second);

void print_blocks();

mword *head = NULL;
mword *tail = NULL;
mword *brk_val = NULL;

void *my_malloc(unsigned long size)
{
    mword block_size = (size + 3) & ~BLOCKSIZE_MASK;
    if (!brk_val | !head | !tail)
    {
        if (!initial_heap_setup(block_size))
            return NULL;
    }

    mword *match = first_match(block_size);
    if (!match)
    {
        if (!expand_tail(block_size))
            return NULL;
        match = tail;
    }
    split_and_alloc(match, block_size);
    return match;
}

int my_free(void *address)
{
    mword *block_start = (mword *)address;
    if (block_start < head || block_start > tail)
        return 101;

    if (!get_present(block_start))
        return 102;

    mword block_size = get_size(block_start);
    if (*(block_start - 1) != *(block_start + (block_size >> 2)))
        return 103;
    free_and_merge(block_start);
    return 0;
}

bool initial_heap_setup(mword first_block_size)
{
    brk_val = (mword *)nbrk(NULL);
    head = brk_val + 1;
    if (!nbrk(brk_val + (first_block_size >> 2) + 2))
    {
        return false;
    }
    brk_val = brk_val + (first_block_size >> 2) + 2;
    tail = head;

    set_size(head, first_block_size);
    set_present(head, false);
    return true;
}

mword *first_match(mword size)
{
    mword *cur = head;

    while (cur != tail)
    {
        if (!get_present(cur) && (get_size(cur) >= size))
            return cur;
        cur = get_next(cur);
    }

    if (!get_present(cur) && (get_size(cur) >= size))
        return cur;

    return NULL;
}

bool expand_tail(mword block_size)
{
    if (get_present(tail))
    {
        if (!nbrk(brk_val + (block_size >> 2) + 2))
            return false;
        brk_val = brk_val + (block_size >> 2) + 2;
        tail = get_next(tail);
    }
    else
    {
        mword needed = block_size - get_size(tail);
        if (!nbrk(brk_val + (needed >> 2)))
            return false;
        brk_val = brk_val + (needed >> 2);
    }

    set_size(tail, block_size);
    set_present(tail, false);
    return true;
}

inline mword get_size(mword *block_start)
{
    return *(block_start - 1) & ~BLOCKSIZE_MASK;
}

inline bool get_present(mword *block_start)
{
    return (*(block_start - 1) & BLOCKSIZE_MASK);
}

void set_size(mword *block_start, mword size)
{
    size = (size + 3) & ~BLOCKSIZE_MASK;
    *(block_start - 1) = size;
    *(block_start + (size >> 2)) = size;
}

void set_present(mword *block_start, bool present)
{
    mword size = get_size(block_start) >> 2;
    *(block_start - 1) = (*(block_start - 1) & ~BLOCKSIZE_MASK) | present;
    *(block_start + size) = (*(block_start + size) & ~BLOCKSIZE_MASK) | present;
}

inline mword *get_next(mword *block_start)
{
    return get_block_end(block_start) + 2;
}

inline mword *get_block_start(mword *block_end_meta)
{
    return block_end_meta - (*block_end_meta >> 2);
}

inline mword *get_block_end(mword *block_start)
{
    return block_start + (get_size(block_start) >> 2);
}

inline mword *get_previous(mword *block_start)
{
    return get_block_start(block_start - 2);
}

void split_and_alloc(mword *block_start, mword req_size)
{
    mword full_size = get_size(block_start);
    mword remain_size = full_size - req_size;
    if (remain_size < 2 * sizeof(mword))
    {
        set_present(block_start, true);
        return;
    }

    set_size(block_start, req_size);
    set_present(block_start, true);

    mword *remain_block = get_next(block_start);
    set_size(remain_block, remain_size - 2 * sizeof(mword));
    set_present(remain_block, false);
    if (block_start == tail)
        tail = remain_block;
}

void free_and_merge(mword *block_start)
{
    set_present(block_start, false);
    if (block_start != tail)
    {
        mword *next = get_next(block_start);
        if (!get_present(next))
        {
            merge_blocks(block_start, next);
            if (next == tail)
                tail = block_start;
        }
    }
    if (block_start != head)
    {
        mword *prev = get_previous(block_start);
        if (!get_present(prev))
        {
            merge_blocks(prev, block_start);
            if (block_start == tail)
                tail = prev;
        }
    }
}

void merge_blocks(mword *first, mword *second)
{
    mword merge_size = get_size(first) + get_size(second) + 2 * sizeof(mword);
    set_size(first, merge_size);
    set_present(first, false);
}

void print_blocks()
{
    printf("BLOCKS\n");
    printf("Tail: %p\n", tail);
    mword *cur = head;
    while (cur != tail)
    {
        printf("%p - size: 0x%lx, present: %d\n", cur, get_size(cur), get_present(cur));
        cur = get_next(cur);
    }
    printf("%p - size: 0x%lx, present: %d\n", cur, get_size(cur), get_present(cur));
}
