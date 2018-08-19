/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBDCJIT_DC_JIT_H
#define LIBDCJIT_DC_JIT_H
#pragma once

/*
 * Components functions implemented by the platform (OS, for memory management
 * to allocate pages and change page access rights) and arch (for actual
 * machine code geneeration).
 *
 * These are used by dc_jit.c to implement a backend for JIT compiling
 * calculations.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define DCJIT_CDECL __attribute__((cdecl))
#else
#define DCJIT_CDECL _cdecl
#endif

/* Implemented by the platform memory backend. This is mmap on Unix. */
struct DC_JIT_Page;
unsigned DC_JIT_PageSize();

/* Allocs a page with write-only permissions. */
struct DC_JIT_Page *DC_JIT_AllocPage();
void *DC_JIT_GetPageData(struct DC_JIT_Page *);

/* Marks a page as read/execute only. */
void DC_JIT_MarkPageExecutable(struct DC_JIT_Page *);

/* Clears a page, and marks as write-only again. */
void DC_JIT_RenewPage(struct DC_JIT_Page *);

void DC_JIT_FreePage(struct DC_JIT_Page *);

/* Implemented by the JIT/ASM backend. */
extern const unsigned DC_ASM_jmp_size;
unsigned DCJIT_CDECL DC_ASM_WriteJMP(void *asm_dest, void *jmp_dest);

extern const unsigned DC_ASM_immediate_size;
unsigned DCJIT_CDECL DC_ASM_WriteImmediate(void *dest, float value);

extern const unsigned DC_ASM_push_arg_size;
unsigned DCJIT_CDECL DC_ASM_WritePushArg(void *dest,
    unsigned short arg_num);

extern const unsigned DC_ASM_pop_size;
unsigned DCJIT_CDECL DC_ASM_WritePop(void *dest);

extern const unsigned DC_ASM_add_size;
unsigned DCJIT_CDECL DC_ASM_WriteAdd(void *dest);

extern const unsigned DC_ASM_sub_size;
unsigned DCJIT_CDECL DC_ASM_WriteSub(void *dest);

extern const unsigned DC_ASM_sub_size;
unsigned DCJIT_CDECL DC_ASM_WriteMul(void *dest);

extern const unsigned DC_ASM_div_size;
unsigned DCJIT_CDECL DC_ASM_WriteDiv(void *dest);

extern const unsigned DC_ASM_sin_size;
unsigned DCJIT_CDECL DC_ASM_WriteSin(void *dest);

extern const unsigned DC_ASM_cos_size;
unsigned DCJIT_CDECL DC_ASM_WriteCos(void *dest);

extern const unsigned DC_ASM_sqrt_size;
unsigned DCJIT_CDECL DC_ASM_WriteSqrt(void *dest);

extern const unsigned DC_ASM_ret_size;
unsigned DCJIT_CDECL DC_ASM_WriteRet(void *dest);

void DCJIT_CDECL DC_ASM_Calculate(const void *addr, const float *args, float *result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_JIT_H */
