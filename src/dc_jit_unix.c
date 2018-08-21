/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dc_jit.h"

/* This gets us MAP_ANONYMOUS on newer Linux. */
#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <unistd.h>
#include <sys/mman.h>

static unsigned page_size(){
#ifdef _SC_PAGESIZE
    return sysconf(_SC_PAGESIZE);
#else
    return sysconf(PAGESIZE);
#endif
}

#ifdef MAP_ANONYMOUS
    #define DC_MAP_ANON MAP_ANONYMOUS
#elif defined MAP_ANON
    #define DC_MAP_ANON MAP_ANON
#else
    #error Fix the defines before including mman so that we get MAP_ANONYMOUS
#endif


unsigned DC_JIT_PageSize(){
    static unsigned size = 0;
    if(size == 0)
        size = page_size(); 
    return size;
}

/* Allocs a page with write-only permissions. */
struct DC_JIT_Page *DC_JIT_AllocPage(){
    const unsigned page_size = DC_JIT_PageSize();
    return mmap(NULL, page_size, PROT_WRITE, MAP_PRIVATE|DC_MAP_ANON, -1, 0);
}

void *DC_JIT_GetPageData(struct DC_JIT_Page *p){
    return p;
}

/* Marks a page as read/execute only. */
void DC_JIT_MarkPageExecutable(struct DC_JIT_Page *p){
    const unsigned page_size = DC_JIT_PageSize();
    mprotect(p, page_size, PROT_READ|PROT_EXEC);
}

/* Clears a page, and marks as write-only again. */
void DC_JIT_RenewPage(struct DC_JIT_Page *p){
    const unsigned page_size = DC_JIT_PageSize();
    mprotect(p, page_size, PROT_WRITE);
}

void DC_JIT_FreePage(struct DC_JIT_Page *p){
    const unsigned page_size = DC_JIT_PageSize();
    munmap(p, page_size);
}
