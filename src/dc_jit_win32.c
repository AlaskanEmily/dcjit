/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dc_jit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

unsigned DC_JIT_PageSize(){
    static DWORD page_size = 0;
    if(page_size == 0){
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        page_size = system_info.dwPageSize;
    }
    return page_size;
}

/* Allocs a page with write-only permissions. */
struct DC_JIT_Page *DC_JIT_AllocPage(){
    return VirtualAlloc(NULL, DC_JIT_PageSize(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void *DC_JIT_GetPageData(struct DC_JIT_Page *p){
    return p;
}

/* Marks a page as read/execute only. */
void DC_JIT_MarkPageExecutable(struct DC_JIT_Page *p){
    DWORD unused, page_size = DC_JIT_PageSize();
    if(VirtualProtect(p, page_size, PAGE_EXECUTE_READ, &unused) != 0){
        const DWORD err = GetLastError();
        (void)err;
    }
    FlushInstructionCache(GetCurrentProcess(), p, page_size);
}

/* Clears a page, and marks as write-only again. */
void DC_JIT_RenewPage(struct DC_JIT_Page *p){
    DWORD unused;
    VirtualProtect(p, DC_JIT_PageSize(), PAGE_READWRITE, &unused);
}

void DC_JIT_FreePage(struct DC_JIT_Page *p){
    VirtualFree(p, 0, MEM_RELEASE);
}
