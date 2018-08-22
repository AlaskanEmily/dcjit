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
global DC_ASM_WritePushArg
global _DC_ASM_WritePushArg

global DC_ASM_immediate_size
global DC_ASM_WriteImmediate
global _DC_ASM_WriteImmediate

global DC_ASM_add_size
global DC_ASM_WriteAdd
global _DC_ASM_WriteAdd

global DC_ASM_sub_size
global DC_ASM_WriteSub
global _DC_ASM_WriteSub

global DC_ASM_mul_size
global DC_ASM_WriteMul
global _DC_ASM_WriteMul

global DC_ASM_div_size
global DC_ASM_WriteDiv
global _DC_ASM_WriteDiv

global DC_ASM_sin_size
global DC_ASM_WriteSin
global _DC_ASM_WriteSin

global DC_ASM_cos_size
global DC_ASM_WriteCos
global _DC_ASM_WriteCos

global DC_ASM_sqrt_size
global DC_ASM_WriteSqrt
global _DC_ASM_WriteSqrt

global DC_ASM_add_arg_size
global DC_ASM_WriteAddArg
global _DC_ASM_WriteAddArg

global DC_ASM_sub_arg_size
global DC_ASM_WriteSubArg
global _DC_ASM_WriteSubArg

global DC_ASM_mul_arg_size
global DC_ASM_WriteMulArg
global _DC_ASM_WriteMulArg

global DC_ASM_div_arg_size
global DC_ASM_WriteDivArg
global _DC_ASM_WriteDivArg

global DC_ASM_sin_arg_size
global DC_ASM_WriteSinArg
global _DC_ASM_WriteSinArg

global DC_ASM_cos_arg_size
global DC_ASM_WriteCosArg
global _DC_ASM_WriteCosArg

global DC_ASM_sqrt_arg_size
global DC_ASM_WriteSqrtArg
global _DC_ASM_WriteSqrtArg

global DC_ASM_add_imm_size
global DC_ASM_WriteAddImm
global _DC_ASM_WriteAddImm

global DC_ASM_sub_imm_size
global DC_ASM_WriteSubImm
global _DC_ASM_WriteSubImm

global DC_ASM_mul_imm_size
global DC_ASM_WriteMulImm
global _DC_ASM_WriteMulImm

global DC_ASM_div_imm_size
global DC_ASM_WriteDivImm
global _DC_ASM_WriteDivImm

global DC_ASM_pop_size
global DC_ASM_WritePop
global _DC_ASM_WritePop

global DC_ASM_ret_size
global DC_ASM_WriteRet
global _DC_ASM_WriteRet

global DC_ASM_Calculate
global _DC_ASM_Calculate

; void DC_ASM_WriteJMP(void *asm_dest, void *jmp_dest);
DC_ASM_WriteJMP:
_DC_ASM_WriteJMP:
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    
    mov [eax], WORD 0xFF2E
    mov [eax + 2], edx
    mov eax, 6
    ret

; void DC_ASM_WritePushArg(void *dest, unsigned short arg_num);

DC_ASM_WritePushArg:
_DC_ASM_WritePushArg:
    mov eax, [esp+4] ; get the dest
; Write the 0xF3 0x0F 0x10 prefix that all push ops use.
    mov [eax+0], BYTE 0xF3
    mov [eax+1], BYTE 0x0F
    mov [eax+2], BYTE 0x10
    
; Get the current stack depth
    mov edx, dc_asm_index
    mov ecx, [edx]
    inc DWORD [edx]

; TODO: Check if we have overflowed into the CPU stack.

; Translate the stack depth into the XMM register encoding
    rol cx, 3

; Add 2 to ecx. This forms the opcode movss, xmm, [edx+IMM] or movss xmm, [edx]
    or cl, 2

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
DC_ASM_WriteImmediate:
_DC_ASM_WriteImmediate:
    mov eax, [esp+4] ; Get the dest
    
    ; Get the current stack depth
    mov edx, dc_asm_index
    mov ecx, [edx]
    inc DWORD [edx]
    
    mov edx, [esp+8] ; Get the immediate.
    
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
DC_ASM_WriteSqrt:
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
DC_ASM_WriteAdd:
_DC_ASM_WriteAdd:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_arithmetic
    
; unsigned DCJIT_CDECL DC_ASM_WriteSub(void *dest);
DC_ASM_WriteSub:
_DC_ASM_WriteSub:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteDiv(void *dest);
DC_ASM_WriteDiv:
_DC_ASM_WriteDiv:
    mov ecx, 0xF30F5E00
    jmp dc_asm_write_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteMul(void *dest);
; Mul is last, since it's the most likely and we can avoid a jmp.
DC_ASM_WriteMul:
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
DC_ASM_WritePop:
_DC_ASM_WritePop:
; TODO: Check if we have overflowed into the CPU stack.
    mov eax, [dc_asm_index]
    dec eax
    mov [dc_asm_index], eax
    xor eax, eax
    ret
    
DC_ASM_WriteCosArg:
_DC_ASM_WriteCosArg:
    call _DC_ASM_WritePushArg
    ; FALLTHROUGH
; unsigned DCJIT_CDECL DC_ASM_WriteCos(void *dest);
DC_ASM_WriteCos:
_DC_ASM_WriteCos:
    push 0xFF
    jmp dc_asm_trig_func

DC_ASM_WriteSinArg:
_DC_ASM_WriteSinArg:
    call _DC_ASM_WritePushArg
    ; FALLTHROUGH
; unsigned DCJIT_CDECL DC_ASM_WriteSin(void *dest);
DC_ASM_WriteSin:
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

DC_ASM_WriteSqrtArg:
_DC_ASM_WriteSqrtArg:
    mov eax, dc_asm_index
    mov ecx, [eax]
    inc DWORD [eax]
    ; Get the XMM register
    mov edx, [ecx+dc_asm_unary_codes]
    
    ; Get the destination
    mov eax, [esp+4]
    ; Get the arg number
    mov ecx, [esp+8]
    cmp ecx, 0
    jnz dc_sqrt_nonzero_argument_index
    or edx, 0xF30F5100
    bswap edx
    mov [eax], edx
    mov eax, 4
    ret

dc_sqrt_nonzero_argument_index:
    or edx, 0xF30F5140
    bswap edx
    mov [eax], edx
    rol cx, 2
    mov [eax+4], cx
    mov eax, 5
    ret

DC_ASM_WriteAddArg:
_DC_ASM_WriteAddArg:
    mov ch, 0x58
    jmp dc_asm_write_arg_arithmetic

DC_ASM_WriteSubArg:
_DC_ASM_WriteSubArg:
    mov ch, 0x5C
    jmp dc_asm_write_arg_arithmetic

DC_ASM_WriteDivArg:
_DC_ASM_WriteDivArg:
    mov ch, 0x5E
    jmp dc_asm_write_arg_arithmetic

    ; This is placed at the end, as it is somewhat more likely
DC_ASM_WriteMulArg:
_DC_ASM_WriteMulArg:
    mov ch, 0x59
    ; jmp dc_asm_write_arg_arithmetic

dc_asm_write_arg_arithmetic:
    ; Get the current stack
    mov eax, dc_asm_index
    mov eax, [eax]
    ; Get the XMM register
    mov cl, [eax+dc_asm_unary_codes-1]
    movzx edx, cx
    or edx, 0xF30F0000
    
    ; Get the destination
    mov eax, [esp+4]
    
    ; Get the arg number
    mov ecx, [esp+8]
    
    cmp ecx, 0
    jz dc_asm_write_zero_arg_arithmetic
    shl cl, 2
    or dl, 0x40
    bswap edx
    mov [eax], edx
    mov [eax+4], cl
    mov eax, 5
    ret
dc_asm_write_zero_arg_arithmetic:
    bswap edx
    mov [eax], edx
    mov eax, 4
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteAddImm(void *dest, unsigned short arg);
DC_ASM_WriteAddImm:
_DC_ASM_WriteAddImm:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_imm_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteSubImm(void *dest, unsigned short arg);
DC_ASM_WriteSubImm:
_DC_ASM_WriteSubImm:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_imm_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteDivImm(void *dest, unsigned short arg);
DC_ASM_WriteDivImm:
_DC_ASM_WriteDivImm:
    mov ecx, 0xF30F5E00
    jmp dc_asm_write_imm_arithmetic

; unsigned DCJIT_CDECL DC_ASM_WriteMulImm(void *dest, unsigned short arg);
DC_ASM_WriteMulImm:
_DC_ASM_WriteMulImm:
    mov ecx, 0xF30F5900
    ; jmp dc_asm_write_imm_arithmetic

dc_asm_write_imm_arithmetic:
    ; Write:
    ; push IMM
    ; opss XMM, [esp]
    ; pop eax
    
    ; Get the destination
    mov eax, [esp+4]
    ; Get the immediate
    mov edx, [esp+8]
    mov [eax], BYTE 0x68
    mov [eax+1], edx
    
    ; Get the current stack
    movzx edx, BYTE [dc_asm_index]
    lea edx, [((edx-1) * 8) + 4]
    mov cl, dl
    ; Get the XMM register
    bswap ecx
    mov [eax+5], ecx
    mov [eax+9], WORD 0x5824
    mov eax, 11
    ret

; unsigned DCJIT_CDECL DC_ASM_WriteRet(void *dest);
DC_ASM_WriteRet:
_DC_ASM_WriteRet:
    dec DWORD [dc_asm_index]
    mov eax, [esp+4]
    mov [eax], BYTE 0xC3
    xor eax, eax
    inc eax
    ret

; float DC_ASM_Calculate(void *addr, const float *args, float *result);
DC_ASM_Calculate:
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
    
    ; These indicate (XMM(N), XMM(N-1). Subtract 0xC8 to just get XMM(N)
    dc_asm_arithmetic_codes: db 0xC1,0xCA,0xD3,0xDC,0xE5,0xEE,0xF7
    dc_asm_unary_codes: db 0x02, 0x0A, 0x12, 0x1A, 0x22, 0x2A, 0x32, 0x3A

    DC_ASM_cos_arg_size: ; FALLTHROUGH
    DC_ASM_sin_arg_size: dd 30
    DC_ASM_add_imm_size: ; FALLTHROUGH
    DC_ASM_sub_imm_size: ; FALLTHROUGH
    DC_ASM_div_imm_size: ; FALLTHROUGH
    DC_ASM_mul_imm_size: ; FALLTHROUGH
    DC_ASM_cos_size: ; FALLTHROUGH
    DC_ASM_sin_size: dd 24
    DC_ASM_sqrt_arg_size: ; FALLTHROUGH
    DC_ASM_jmp_size: dd 6
    DC_ASM_add_arg_size: ; FALLTHROUGH
    DC_ASM_sub_arg_size: ; FALLTHROUGH
    DC_ASM_mul_arg_size: ; FALLTHROUGH
    DC_ASM_div_arg_size: ; FALLTHROUGH
    DC_ASM_push_arg_size: dd 5
    DC_ASM_immediate_size: dd 14
    DC_ASM_sub_size: ; FALLTHROUGH
    DC_ASM_mul_size: ; FALLTHROUGH
    DC_ASM_div_size: ; FALLTHROUGH
    DC_ASM_sqrt_size: ; FALLTHROUGH
    DC_ASM_add_size: dd 4
    DC_ASM_ret_size: dd 1
