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
; Unlike x86, we start with a `lea rax,[rsp-0x8]` (actually -16, but then we
; `call`) to leave rax as an address to place immediates in.


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

; unsigned DC_ASM_WritePushArg(void *dest, unsigned short arg_num);
DC_ASM_WritePushArg:
    ; Write:
    ; movss XMM, [rsi+N]
    ; Or:
    ; movss XMM, [rsi]
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    inc DWORD [rax]
    lea rax, [(r8 * 8) + 0xF30F1006]
    shl rsi, 2
    jz push_zero_arg
    add eax, 0x40
    bswap eax
    mov [rdi], eax
    mov ax, si
    mov [rdx+4], al
    mov rax, 5
    ret

push_zero_arg:
    bswap eax
    mov [rdi], eax
    mov rax, 4
    ret

; unsigned DC_ASM_WriteImmediate(void *dest, float value);
DC_ASM_WriteImmediate:
    ; Write the immediate to [rax]:
    ; mov [rax], IMM
    ; Or:
    ; xor edi, edi
    ; mov [rax], edi
    ;
    ; Write:
    ; movss XMM, [rax]
    ;
    ; eax will specify (number of instructions written) - 4 after the split.

    ; Get the XMM code
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    inc DWORD [rax]
    lea edx, [(r8 * 3) + 0xF30F1000]
    bswap edx
    
    ; Get the immediate
    push r8
    movss [rsp], xmm0
    mov eax, [rsp]
    pop r8
    cmp eax, 0
    jz write_zero_imm
    mov [rdi], WORD 0x00C7
    mov [rdi+2], eax
    mov eax, 2
    jmp write_imm
write_zero_imm:
    mov [rdi], DWORD 0x3889FF31 ; xor edi, edi / mov [rax], edi
    xor eax, eax
write_imm:
    mov r8d, eax
    mov [rdi+r8+4], edx
    add edx, 8
    ret

; unsigned DC_ASM_WriteAdd(void *dest);
DC_ASM_WriteAdd:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_arithmetic
    
; unsigned DC_ASM_WriteSub(void *dest);
DC_ASM_WriteSub:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_arithmetic

; unsigned DC_ASM_WriteDiv(void *dest);
DC_ASM_WriteDiv:
    mov ecx, 0xF30F5E00
    jmp dc_asm_write_arithmetic

; unsigned DC_ASM_WriteMul(void *dest);
; Mul is last, since it's the most likely and we can avoid a jmp.
DC_ASM_WriteMul:
_DC_ASM_WriteMul:
    mov ecx, 0xF30F5900
    ; jmp dc_asm_write_arithmetic

dc_asm_write_arithmetic:
    mov rax, QWORD dc_asm_index
    dec DWORD [rax]
    mov edx, [rax]
    mov rax, QWORD dc_asm_arithmetic_codes
    mov r8d, edx
    xor cl, [rax + r8 - 1]
    bswap ecx
    mov [rdi], ecx
    mov eax, 4
    ret

DC_ASM_WriteSin:
    mov cx, 0xFED9
    jmp dc_asm_trig

DC_ASM_WriteCos:
    mov cx, 0xFFD9
    jmp dc_asm_trig

DC_ASM_WriteSqrt:
    mov cx, 0xFAD9

dc_asm_trig:
    ; Write:
    ; movss [rax], XMM
    ; fld [rax] (0x00D9)
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    dec r8
    lea rdx, [(r8d * 8) + 0xF30F1100]
    bswap edx
    mov [rdi], edx
    mov [rdi+4], WORD 0x00D9
    mov [rdi+6], cx
    
    ; Write:
    ; movq2dq XMM, mm0
    ; cvtsd2ss XMM, XMM
    ; fdecstp
    ; ffree st0
    ; r8 still has the XMM register.
    lea eax, [(r8 * 8) + 0xF30FD6C8]
    mov [rdi+10], eax
    ; Write the FPU stuff.
    mov [rdi+14], DWORD 0xF6D9C0DD
    ; Write the cvt
    lea eax, [(r8 * 8) + 0xF20F5ACA]
    mov [rdi+18], eax
    mov rax, 22
    ret

DC_ASM_WritePop:
    mov rax, QWORD dc_asm_index
    dec DWORD [rax]
    ret

DC_ASM_WriteRet:
    mov [rdi], BYTE 0xC3
    mov rax, 1
    ret

; void DC_ASM_Calculate(const void *addr, const float *args, float *result);
DC_ASM_Calculate:
    push rdx
    lea rax,[rsp-0x10]
    call [rdi]
    pop rdx
    movss [rdx], xmm0
    ret

section .bss
    DC_ASM_pop_size: ; FALLTHROUGH
    dc_zero_memory: resd 1
    dc_asm_index: resd 1

section .data
    dc_asm_arithmetic_codes: db 0xC1,0xCA,0xD3,0xDC,0xE5,0xEE,0xF7
    DC_ASM_ret_size: dd 1
    DC_ASM_sin_size: ; FALLTHROUGH
    DC_ASM_cos_size: ; FALLTHROUGH
    DC_ASM_sqrt_size: dd 15
    DC_ASM_push_arg_size: dd 5
    DC_ASM_immediate_size: dd 10
    DC_ASM_jmp_size: dd 13
    DC_ASM_add_size: ; FALLTHROUGH
    DC_ASM_sub_size: ; FALLTHROUGH
    DC_ASM_mul_size: ; FALLTHROUGH
    DC_ASM_div_size: dd 4

