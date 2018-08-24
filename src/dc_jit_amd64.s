; Copyright (c) 2018, Transnat Games
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.
;
; Although the x86 backend always outputs SSE instructions, we can trust SSE2
; to exist on amd64. This allows us to more easily move data to/from the SSE
; registers using the MOVD instructions.
;
; We use the extended, XMM versions of mov instructions from and the cvt
; instructions (double to/from float) from SSE2, and the MOV instructions that
; can store/load from MMX registers for sin/cos and fld1.
;
; See the file dc_jit_win64 for how we handle the 64-bit Windows calling
; convention, which uses different registers for parameter transfer than SysV.
;
; TODO:
; We could easily start with a `lea rax,[rsp-0x8]` to leave rax as an address
; to place immediates.

section .text
bits 64

global DC_ASM_jmp_size
global DC_ASM_WriteJMP

global DC_ASM_push_arg_size
global DC_ASM_WritePushArg

global DC_ASM_immediate_size
global DC_ASM_WriteImmediate

global DC_ASM_add_size
global DC_ASM_WriteAdd

global DC_ASM_sub_size
global DC_ASM_WriteSub

global DC_ASM_mul_size
global DC_ASM_WriteMul

global DC_ASM_div_size
global DC_ASM_WriteDiv

global DC_ASM_sin_size
global DC_ASM_WriteSin

global DC_ASM_cos_size
global DC_ASM_WriteCos

global DC_ASM_sqrt_size
global DC_ASM_WriteSqrt

global DC_ASM_add_arg_size
global DC_ASM_WriteAddArg

global DC_ASM_sub_arg_size
global DC_ASM_WriteSubArg

global DC_ASM_mul_arg_size
global DC_ASM_WriteMulArg

global DC_ASM_div_arg_size
global DC_ASM_WriteDivArg

global DC_ASM_sin_arg_size
global DC_ASM_WriteSinArg

global DC_ASM_cos_arg_size
global DC_ASM_WriteCosArg

global DC_ASM_sqrt_arg_size
global DC_ASM_WriteSqrtArg

global DC_ASM_add_imm_size
global DC_ASM_WriteAddImm

global DC_ASM_sub_imm_size
global DC_ASM_WriteSubImm

global DC_ASM_mul_imm_size
global DC_ASM_WriteMulImm

global DC_ASM_div_imm_size
global DC_ASM_WriteDivImm

global DC_ASM_pop_size
global DC_ASM_WritePop

global DC_ASM_ret_size
global DC_ASM_WriteRet

global DC_ASM_Calculate

DC_ASM_WriteJMP:
    ; There are no absolute 64-bit jmps, so we push the address then ret.
    ; Write:
    ; mov r8, ADDR
    ; push r8
    ; ret
    mov [rdi], WORD 0xB849
    mov [rdi+2], rsi
    mov [rdi+10], WORD 0x5041
    mov [rdi+12], BYTE 0xC3
    mov rax, 13
    ret

dc_asm_push_zero_arg:
    ; Write: movss XMM, [rsi]
    bswap eax
    mov [rdi], eax
    mov rax, 4
    ret

DC_ASM_WritePushArg:
    ; We need to test for a zero arg.
    mov rax, dc_asm_index
    mov r8d, DWORD [rax]
    inc DWORD [rax]
    
    movzx eax, BYTE [dc_movss_encoding+r8]
    or eax, 0xF30F10
    
    rol esi, 2
    jz dc_asm_push_zero_arg
    ; Write: movss XMM, [rsi+OFFSET]
    
    add al, 0x40
    bswap eax
    mov [rdi], eax
    mov [rdi+4], si
    mov rax, 5
    ret

    ; Write:
    ; xor r8, r8
    ; push r8
    ; mov XMM, [rsp]
    

DC_ASM_WriteImmediate:
    ; rax holds our current offset
    ; Check for zero
    ucomiss xmm0, [dc_zero_memory]
    jz push_zero_immediate
    ; Write:
    ; mov r8, IMM64
    mov [rdi+0], BYTE 0x49
    mov [rdi+1], BYTE 0xB8
    push rax
    cvtss2sd xmm1, xmm0
    movsd [rsp], xmm1
    pop rax
    mov [rdi+2], rax
    mov rax, 11
    
    jmp push_rejoin
push_zero_immediate:
    ; Write xor r8, r8
    mov [rdi+0], BYTE 0x4D
    mov [rdi+1], BYTE 0x31
    mov [rdi+2], BYTE 0xC0
    mov rax, 3

push_rejoin:
    ; push r8
    mov [rdi+rax], WORD 0x5041
    ; cvtsd2ss XMM, [rax]
    mov rdx, dc_asm_index
    inc DWORD [rdx]
    mov r8d, [rdx]
    lea edx, [(r8 * 8)+0xF20F5A04]
    bswap edx
    mov [rdi+rax+2], edx
    ; pop r8
    mov [rdi+rax+6], WORD 0x5841
    add eax, 8
    ret

section .bss
    dc_zero_memory: resd 1
    dc_asm_index: resd 1
    DC_ASM_pop_size: resd 1

section .data
    dc_movss_encoding: dd 0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E
    DC_ASM_immediate_size: dd 19
    DC_ASM_jmp_size: dd 13
    DC_ASM_push_arg_size: dd 5

