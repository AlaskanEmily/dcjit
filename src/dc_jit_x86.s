; Copyright (c) 2018, Transnat Games
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

section .text
bits 32

global DC_ASM_jmp_size
global _DC_ASM_WriteJMP

global DC_ASM_push_arg_size
global _DC_ASM_WritePushArg

global DC_ASM_immediate_size
global _DC_ASM_WriteImmediate

global DC_ASM_add_size
global _DC_ASM_WriteAdd

global DC_ASM_sub_size
global _DC_ASM_WriteSub

global DC_ASM_mul_size
global _DC_ASM_WriteMul

global DC_ASM_div_size
global _DC_ASM_WriteDiv

global DC_ASM_pop_size
global _DC_ASM_WritePop

global DC_ASM_ret_size
global _DC_ASM_WriteRet

global _DC_ASM_Calculate

; void DC_ASM_WriteJMP(void *asm_dest, void *jmp_dest);
_DC_ASM_WriteJMP:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    
    mov [eax], WORD 0xFF2E
    mov [eax + 2], edx
    mov eax, 6
    ret

; void DC_ASM_WritePushArg(void *dest, unsigned short arg_num);

_DC_ASM_WritePushArg:
    mov eax, [esp+4] ; get the dest
; Write the 0xF3 0x0F 0x10 prefix that all push ops use.
    mov [eax+0], BYTE 0xF3
    mov [eax+1], BYTE 0x0F
    mov [eax+2], BYTE 0x10
    
; Get the current stack depth
    mov ecx, [dc_asm_index]
    inc ecx
    mov [dc_asm_index], ecx
    dec ecx

; TODO: Check if we have overflowed into the CPU stack.

; Translate the stack depth into the XMM register encoding
    shl ecx, 3

; Add 2 to ecx. This forms the opcode movss, xmm, [edx+IMM] or movss xmm, [edx]
    or ecx, 2

; Get the arg number
    xor edx, edx
    or dx, [esp+8]
    jz push_zero
    
; TODO: Check for larger arg numbers
    
; Change arg number into an offset
    shl edx, 2
    
    or ecx, 0x40
    mov [eax+3], cl
    mov [eax+4], dl
    mov eax, 5
    ret
push_zero:
    mov [eax+3], ecx
    mov eax, 4
    ret

; unsigned DC_ASM_WriteImmediate(void *dest, float value);
_DC_ASM_WriteImmediate:
    mov eax, [esp+4] ; Get the dest
    mov edx, [esp+8] ; Get the immediate.
    
    ; TODO: FP zeroes could use xor eax, eax instead of a mov eax, IMM
    
    ; Write mov eax, IMM ; push eax ; mov xmm, [esp] ; pop eax
    mov [eax], DWORD 0xB8 ; mov eax,
    mov [eax+1], edx ; IMM for move
    mov [eax+5], DWORD 0x50 ; push eax
    
    ; Get the current stack depth
    mov ecx, [dc_asm_index]
    inc ecx
    mov [dc_asm_index], ecx
    dec ecx
; Translate the stack depth into the XMM register encoding
    shl ecx, 3
    or ecx, 4
    
; Write the movss (0xF3 0x0F 0x10 MM 0x24)
    mov [eax+6], BYTE 0xF3
    mov [eax+7], BYTE 0x0F
    mov [eax+8], BYTE 0x10
    mov [eax+9], BYTE cl
    mov [eax+10], BYTE 0x24
; pop eax
    mov [eax+11], BYTE 0x58
    mov eax, 12
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteAdd(void *dest);
_DC_ASM_WriteAdd:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_arithmetic
    
; unsigned DCJIT_CDECL DC_ASM_WriteSub(void *dest);
_DC_ASM_WriteSub:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteMul(void *dest);
_DC_ASM_WriteMul:
    mov ecx, 0xF30F5900
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteDiv(void *dest);
_DC_ASM_WriteDiv:
    mov ecx, 0xF30F5E00
    ; jmp dc_asm_write_arithmetic

dc_asm_write_arithmetic:
    mov eax, [dc_asm_index]
    dec eax
    mov cl, [dc_asm_arithmetic_codes + eax - 1]
    mov [dc_asm_index], eax
    bswap ecx
    mov edx, [esp+4]
    mov [edx], ecx
    mov eax, 4
    ret

; unsigned DC_ASM_WritePop(void *dest, unsigned short arg_num);
_DC_ASM_WritePop:
; TODO: Check if we have overflowed into the CPU stack.
    mov eax, [dc_asm_index]
    dec eax
    mov [dc_asm_index], eax
    xor eax, eax
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteRet(void *dest);
_DC_ASM_WriteRet:
    dec DWORD [dc_asm_index]
    mov eax, [esp+4]
    mov [eax], BYTE 0xC3
    xor eax, eax
    inc eax
    ret

; float DC_ASM_Calculate(void *addr, const float *args, float *result);
_DC_ASM_Calculate:
    mov edx, [esp+8]
    call [esp+4]
    mov eax, [esp+12]
    movss [eax], xmm0
    ret

section .bss
    dc_asm_index: resd 1
    DC_ASM_pop_size: resd 1

section .data
    
    dc_asm_arithmetic_codes: db 0xC1,0xCA,0xD3,0xDC,0xE5,0xEE,0xF7
    
    DC_ASM_jmp_size: dd 6
    DC_ASM_push_arg_size: dd 5
    DC_ASM_immediate_size: dd 12
    DC_ASM_sub_size: ; FALLTHROUGH
    DC_ASM_mul_size: ; FALLTHROUGH
    DC_ASM_div_size: ; FALLTHROUGH
    DC_ASM_add_size: dd 4
    DC_ASM_ret_size: dd 1
