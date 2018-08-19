/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dc_jit.h"
#include "dc_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NUM_PAGE_LISTS 2

struct DC_X_PageList {
    struct DC_JIT_Page *page;
    unsigned at, refs;
    struct DC_X_PageList *next;
};

struct DC_X_Context{
    unsigned page_size;
    struct DC_X_PageList *next_page, *active_pages, *free_pages;
};

struct DC_X_Calculation{
    struct DC_X_PageList *page;
    unsigned start;
};

struct DC_X_CalculationBuilder{
    unsigned at;
    /* Data follows. */
};

#define DC_X_GET_BUILDER_BYTES(BLD) ((unsigned char*)((BLD)+1))
#define DC_X_GET_BUILDER_AT(BLD) (((unsigned char*)((BLD)+1)) + ((BLD)->at))

struct DC_X_Context *DC_X_CreateContext(void){
    struct DC_X_Context *const ctx = calloc(sizeof(struct DC_X_Context), 1);
    ctx->page_size = DC_JIT_PageSize();
    return ctx;
}

void DC_X_FreeContext(struct DC_X_Context *ctx){
    struct DC_X_PageList *page_lists[NUM_PAGE_LISTS];
    unsigned i = 0;
    
    page_lists[0] = ctx->active_pages;
    page_lists[1] = ctx->free_pages;
    
    if(ctx->next_page != NULL){
        assert(ctx->next_page->next == NULL);
        assert(ctx->next_page->page != NULL);
        DC_JIT_FreePage(ctx->next_page->page);
        free(ctx->next_page);
    }
    
    do{
        struct DC_X_PageList *page = page_lists[i];
        while(page != NULL){
            struct DC_X_PageList *const next = page->next;
            assert(page->page != NULL);
            DC_JIT_FreePage(page->page);
            free(page);
            page = next;
        }
    }while(++i < NUM_PAGE_LISTS);
    free(ctx);
}

struct DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(
    struct DC_X_Context *ctx){
    
    struct DC_X_CalculationBuilder *const builder =
        malloc(sizeof(struct DC_X_CalculationBuilder) + ctx->page_size);
    builder->at = 0;
    return builder;
}

void DC_X_BuildPushImmediate(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float value){
    (void)ctx;
    bld->at += DC_ASM_WriteImmediate(DC_X_GET_BUILDER_AT(bld), value);
}

void DC_X_BuildPushArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg_num){
    (void)ctx;
    bld->at += DC_ASM_WritePushArg(DC_X_GET_BUILDER_AT(bld), arg_num);
}

#define DC_X_OP(NAME)\
void DC_X_Build ## NAME(struct DC_X_Context *ctx,\
    struct DC_X_CalculationBuilder *bld){\
    (void)ctx;\
    bld->at += DC_ASM_Write ## NAME(DC_X_GET_BUILDER_AT(bld));\
}

DC_X_OP(Add)
DC_X_OP(Sub)
DC_X_OP(Mul)
DC_X_OP(Div)
DC_X_OP(Sin)
DC_X_OP(Cos)
DC_X_OP(Sqrt)
DC_X_OP(Pop)

void DC_X_AbandonCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld){
    (void)ctx;
    free(bld);
}

struct DC_X_Calculation *DC_X_FinalizeCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld){
    
    struct DC_JIT_Page *const new_page = DC_JIT_AllocPage();
    bld->at += DC_ASM_WriteRet(DC_X_GET_BUILDER_AT(bld));

#if 0
    { /* For debugging only. */
        FILE *const log = fopen("log.bin", "wb");
        fwrite(DC_X_GET_BUILDER_BYTES(bld), 1, bld->at, log);
        fclose(log);
    }
#endif
    
    memcpy(DC_JIT_GetPageData(new_page),
        DC_X_GET_BUILDER_BYTES(bld),
        ctx->page_size);
    
    DC_JIT_MarkPageExecutable(new_page);
    
    free(bld);
    {
        struct DC_X_Calculation *const calc =
            malloc(sizeof(struct DC_X_Calculation));
        struct DC_X_PageList *const pagelist =
            malloc(sizeof(struct DC_X_PageList));
        /* Stitch this new page into the start of the context's pagelist. */
        pagelist->next = ctx->active_pages;
        ctx->active_pages = pagelist;
        pagelist->refs = 1;
        pagelist->page = new_page;
        
        calc->page = pagelist;
        calc->start = 0;
        
        return calc;
    }
}

void DC_X_Free(struct DC_X_Context *ctx, struct DC_X_Calculation *calc){
    if((calc->page->refs -= 1) == 0){
        if(ctx->active_pages == calc->page){
            ctx->active_pages = ctx->active_pages->next;
            calc->page->next = ctx->free_pages;
            ctx->free_pages = calc->page;
        }
        else{
            struct DC_X_PageList *page = ctx->active_pages->next,
                **prev = &(ctx->active_pages);
            
            while(page != calc->page){
                prev = &(page->next);
                page = page->next;
            }
            
            prev[0] = page->next;
            page->next = ctx->free_pages;
            ctx->free_pages = page;
        }
    }
    
    free(calc);
}

float DC_X_Calculate(const struct DC_X_Calculation *calc, const float *args){
    const unsigned char *const code = DC_JIT_GetPageData(calc->page->page);
    float r;
    DC_ASM_Calculate(code + calc->start, args, &r);
    return r;
}
