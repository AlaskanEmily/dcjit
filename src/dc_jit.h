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


#if defined WIN64 || defined _WIN64 || ( defined __CYGWIN__ && defined __amd64 )
    #define C_DEMANGLE_NAME(X) X ## _Win64
    #define DCJIT_IS_WINDOWS 1
#else
    #define C_DEMANGLE_NAME(X) X
    #define DCJIT_IS_WINDOWS 0
#endif

#ifdef __GNUC__
    #if DCJIT_IS_WINDOWS
        #define DCJIT_CDECL(X) __attribute__((cdecl)) C_DEMANGLE_NAME(X)
    #else
        #define DCJIT_CDECL(X) X
    #endif
#else
    #define DCJIT_CDECL(X) _cdecl C_DEMANGLE_NAME(X)
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
unsigned DCJIT_CDECL(DC_ASM_WriteJMP)(void *asm_dest, void *jmp_dest);

extern const unsigned DC_ASM_immediate_size;
unsigned DCJIT_CDECL(DC_ASM_WriteImmediate)(void *dest, float value);

extern const unsigned DC_ASM_push_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WritePushArg)(void *dest,
    unsigned short arg_num);

extern const unsigned DC_ASM_pop_size;
unsigned DCJIT_CDECL(DC_ASM_WritePop)(void *dest);

extern const unsigned DC_ASM_add_size;
unsigned DCJIT_CDECL(DC_ASM_WriteAdd)(void *dest);

extern const unsigned DC_ASM_sub_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSub)(void *dest);

extern const unsigned DC_ASM_mul_size;
unsigned DCJIT_CDECL(DC_ASM_WriteMul)(void *dest);

extern const unsigned DC_ASM_div_size;
unsigned DCJIT_CDECL(DC_ASM_WriteDiv)(void *dest);

extern const unsigned DC_ASM_sin_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSin)(void *dest);

extern const unsigned DC_ASM_cos_size;
unsigned DCJIT_CDECL(DC_ASM_WriteCos)(void *dest);

extern const unsigned DC_ASM_sqrt_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSqrt)(void *dest);

extern const unsigned DC_ASM_ret_size;
unsigned DCJIT_CDECL(DC_ASM_WriteRet)(void *dest);

extern const unsigned DC_ASM_add_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteAddArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_sub_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSubArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_mul_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteMulArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_div_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteDivArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_sin_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSinArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_cos_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteCosArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_sqrt_arg_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSqrtArg)(void *dest, unsigned short arg);

extern const unsigned DC_ASM_add_imm_size;
unsigned DCJIT_CDECL(DC_ASM_WriteAddImm)(void *dest, float imm);

extern const unsigned DC_ASM_sub_imm_size;
unsigned DCJIT_CDECL(DC_ASM_WriteSubImm)(void *dest, float imm);

extern const unsigned DC_ASM_mul_imm_size;
unsigned DCJIT_CDECL(DC_ASM_WriteMulImm)(void *dest, float imm);

extern const unsigned DC_ASM_div_imm_size;
unsigned DCJIT_CDECL(DC_ASM_WriteDivImm)(void *dest, float imm);

extern const unsigned DC_ASM_ret_size;
unsigned DCJIT_CDECL(DC_ASM_WriteRet)(void *dest);

void DCJIT_CDECL(DC_ASM_Calculate)(const void *addr, const float *args, float *result);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_JIT_H */
