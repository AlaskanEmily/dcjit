// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Implements the JIT's memory allocation/protection functions using the native
// Haiku API. If someone really wants to compile this for BeOS, please add a
// preprocessor check for BeOS and not Haiku, and define B_EXECUTABLE_AREA to
// zero in that case (all readable data was executable in BeOS, as the NX bit
// on x86 hadn't been invented yet).

#include "dc_jit.h"

// For BeOS, this would also have to have be in front of the path.
#include <kernel/OS.h>

// Haiku/BeOS exposes a way to allocate whole pages, which are called areas.
// This is quite similar to how mmap with a -1 FD and MAP_ANONYMOUS works, but
// having read the source for Haiku's implementation, it's quite a bit simpler.
// It also gives us an opportunity to show our love for Haiku.
struct DC_JIT_Page{
    DC_JIT_Page()
      : m_id(B_ERROR)
      , m_data(NULL){
        m_id = create_area("DCJIT generated code",
            &m_data,
            B_ANY_ADDRESS,
            B_PAGE_SIZE,
            B_NO_LOCK,
            B_WRITE_AREA);
    }
    
    ~DC_JIT_Page(){
        if(ok())
            delete_area(m_id);
    }
    
    inline bool ok() const {
        return m_id != B_ERROR && m_id != B_BAD_VALUE && m_id != B_NO_MEMORY;
    }
    
    inline void makeExecutable(){
        set_area_protection(m_id, B_READ_AREA|B_EXECUTE_AREA);
    }
    
    inline void renew(){
        set_area_protection(m_id, B_WRITE_AREA);
    }
    
    inline void *data() { return m_data; }
    
private:
    
    area_id m_id;
    void *m_data;
};

unsigned DC_JIT_PageSize(){
    return B_PAGE_SIZE;
}

/* Allocs a page with write-only permissions. */
DC_JIT_Page *DC_JIT_AllocPage(){
    DC_JIT_Page *const page = new DC_JIT_Page();
    if(page == NULL || page->ok()){
        return page;
    }
    else{
        delete page;
        return NULL;
    }
}

void *DC_JIT_GetPageData(DC_JIT_Page *p){
    return p->data();
}

/* Marks a page as read/execute only. */
void DC_JIT_MarkPageExecutable(DC_JIT_Page *p){
    p->makeExecutable();
}

/* Clears a page, and marks as write-only again. */
void DC_JIT_RenewPage(DC_JIT_Page *p){
    p->renew();
}

void DC_JIT_FreePage(DC_JIT_Page *p){
    delete p;
}
