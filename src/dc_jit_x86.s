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

global DC_ASM_sin_size
global _DC_ASM_WriteSin

global DC_ASM_cos_size
global _DC_ASM_WriteCos

global DC_ASM_sqrt_size
global _DC_ASM_WriteSqrt

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

dc_asm_immediate_zero:
    ; Write:
    ; xor eax, eax
    ; push eax
    ; movss XMM, [esp]
    ; pop eax
    mov [eax], DWORD 0xF350C031
    shl cx, 11
    or ecx, 0x0F100424
    bswap ecx
    mov [eax+4], ecx
    mov [eax+8], BYTE 0x58
    mov eax, 9
    ret

dc_asm_immediate_one:
    ; write:
    ; fld1
    ; fstp [esp-4]
    ; movss xmm0, [esp-4]
    mov [eax], WORD 0xE8D9
    mov [eax+2], DWORD 0xFC245CD9
    shl cl, 3
    add cl, 0x44
    or ecx, 0xF30F1000
    bswap ecx
    mov [eax+6], ecx
    mov [eax+10], WORD 0xFC24 
    mov eax, 12
    ret

; unsigned DC_ASM_WriteImmediate(void *dest, float value);
_DC_ASM_WriteImmediate:
    mov eax, [esp+4] ; Get the dest
    mov edx, [esp+8] ; Get the immediate.
    
    ; Get the current stack depth
    mov ecx, [dc_asm_index]
    inc ecx
    mov [dc_asm_index], ecx
    dec ecx
    
    cmp edx, 0
    je dc_asm_immediate_zero
    
    cmp edx, 0x3F800000
    je dc_asm_immediate_one
    
    ; Write:
    ; lea eax,[esp-0x4]
    ; mov [eax], IMM
    ; movss  xmm0, [eax]
    ;
    ; This is:
    ; 0x24FC8D44 (lea eax,[esp-0x4])
    ; 0xC700 IMM (mov [eax], IMM)
    ; 0xF30F10 (XMM)
    
    mov [eax], DWORD 0xFC24448D
    mov [eax+4], WORD 0x00C7
    mov [eax+6], edx
    shl cl, 3
    or ecx, 0xF30F1000
    bswap ecx
    mov [eax+10], ecx
    mov eax, 14
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteSqrt(void *dest);
; Since this does not change the stack pointer, it can't be folded with the
; other arithmetic ops
_DC_ASM_WriteSqrt:
    mov ecx, 0xF30F5101
    mov eax, [dc_asm_index]
    xor cl, [dc_asm_arithmetic_codes + eax - 1]
    bswap ecx
    mov edx, [esp+4]
    mov [edx], ecx
    mov eax, 4
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteAdd(void *dest);
_DC_ASM_WriteAdd:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_arithmetic
    
; unsigned DCJIT_CDECL DC_ASM_WriteSub(void *dest);
_DC_ASM_WriteSub:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteDiv(void *dest);
_DC_ASM_WriteDiv:
    mov ecx, 0xF30F5E00
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteMul(void *dest);
; Mul is last, since it's the most likely and we can avoid a jmp.
_DC_ASM_WriteMul:
    mov ecx, 0xF30F5900
    ; jmp dc_asm_write_arithmetic

dc_asm_write_arithmetic:
    mov eax, [dc_asm_index]
    dec eax
    xor cl, [dc_asm_arithmetic_codes + eax - 1]
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

; unsigned DCJIT_CDECL DC_ASM_WriteCos(void *dest);
_DC_ASM_WriteCos:
    push 0xFF
    jmp dc_asm_trig_func

; unsigned DCJIT_CDECL DC_ASM_WriteSin(void *dest);
_DC_ASM_WriteSin:
    push 0xFE
    ; dc_asm_trig_func

dc_asm_trig_func:
    ; Check for SSE2, and use double-precision trig functions if the better
    ; conversions functions are available.
    push ebx
    xor eax, eax
    inc eax
    cpuid
    pop ebx
    mov eax, [esp+8]
    mov ecx, [dc_asm_index]
    dec ecx
    bt edx, 26
    jnc dc_asm_x87_trig
    jmp dc_asm_x87_trig
dc_asm_sse2_trig:
    ret
    
dc_asm_x87_trig:
    ; It's slightly more efficient to move the value of esp into eax, since we
    ; dereference the value so much.
    ; Write:
    ; lea eax, [esp-4]
    ; movss [eax], XMM
    ; fld (DWORD) [eax]
    ; fsin
    ; fstp (DWORD) [eax]
    ; movss xmm0, [eax]
    mov [eax], DWORD 0xFC24448D ; lea eax, [esp-4]
    shl cl, 3
    or ecx, 0xF30F1100
    bswap ecx
    mov [eax+4], ecx ; movss [eax], XMM
    pop edx ; Get the sin/cos byte
    shl edx, 24
    or edx, 0x00D900D9
    mov [eax+8], edx ; fld (DWORD) [eax], f(cos|sin)
    mov [eax+12], WORD 0x18D9
    bswap ecx
    mov ch, 0x10
    bswap ecx
    mov [eax+14], ecx
    mov eax, 18
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
    
    DC_ASM_cos_size: ; FALLTHROUGH
    DC_ASM_sin_size: dd 24
    DC_ASM_jmp_size: dd 6
    DC_ASM_push_arg_size: dd 5
    DC_ASM_immediate_size: dd 14
    DC_ASM_sub_size: ; FALLTHROUGH
    DC_ASM_mul_size: ; FALLTHROUGH
    DC_ASM_div_size: ; FALLTHROUGH
    DC_ASM_sqrt_size: ; FALLTHROUGH
    DC_ASM_add_size: dd 4
    DC_ASM_ret_size: dd 1
