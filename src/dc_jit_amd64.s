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
;
; Any functions that need x87 (sin/cos) use different code gen if the CPU is
; detected to be an AMD processor. This is because 32-bit fld/fstp and movss is
; slightly slower than a cvtss2sd and 64-bit fld/fstp on AMD, but it's
; definitely the opposite on an Intel CPU.

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
    lea rax, [rsp-8]
    movss [rax], xmm0
    mov eax, [rax]
    cmp eax, 0
    je write_zero_imm
    mov [rdi], WORD 0x00C7
    mov [rdi+2], eax
    mov rax, 6
    jmp write_imm
write_zero_imm:
    mov [rdi], DWORD 0x3889FF31 ; xor edi, edi / mov [rax], edi
    mov rax, 4
write_imm:
    mov [rdi+rax], edx
    add rax, 4
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
    ; TODO: This code gen is based on a flawed test. It is actually very slow.
    ; We should rejoin this with the amd/intel trig code.
    
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

; unsigned DC_ASM_WriteAddArg(void *dest, unsigned short arg);
DC_ASM_WriteAddArg:
    mov ecx, 0xF30F5806
    jmp dc_asm_write_arg_arithmetic

; unsigned DC_ASM_WriteSubArg(void *dest, unsigned short arg);
DC_ASM_WriteSubArg:
    mov ecx, 0xF30F5C06
    jmp dc_asm_write_arg_arithmetic

; unsigned DC_ASM_WriteDivArg(void *dest, unsigned short arg);
DC_ASM_WriteDivArg:
    mov ecx, 0xF30F5E06
    jmp dc_asm_write_arg_arithmetic

; unsigned DC_ASM_WriteMulArg(void *dest, unsigned short arg);
DC_ASM_WriteMulArg:
    mov ecx, 0xF30F5906
    ; jmp dc_asm_write_arg_arithmetic

dc_asm_write_arg_arithmetic:
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    lea rax, [(r8d * 8) + ecx + 6]
    
    ; TODO: Something up with that calculation above?
    ;or ecx, eax
    
    shl si, 2
    jz dc_asm_write_zero_arg_arithmetic
    add cl, 0x40
    bswap ecx
    mov [rdi], ecx
    mov cx, si
    mov [rdi+4], cl
    mov rax, 5
    ret
    
dc_asm_write_zero_arg_arithmetic:
    bswap ecx
    mov [rdi], ecx
    mov rax, 4
    ret

; unsigned DC_ASM_WriteSinArg(void *dest, float immediate);
DC_ASM_WriteSinArg:
    mov dx, 0xD9FE
    jmp dc_asm_write_trig_arg

; unsigned DC_ASM_WriteCosImm(void *dest, float immediate);
DC_ASM_WriteCosArg:
    mov dx, 0xD9FF
    ; jmp dc_asm_write_trig_arg:

dc_asm_write_trig_arg:
    ; Check the CPU vendor string.
    push rdx
    push rbx
    xor eax, eax
    cpuid
    pop rbx
    pop rdx
    
    ; Grab the current index into r8 right now.
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    inc r8
    mov [rax], r8d
    dec r8
    
    ; ecx has the last 4 letters of the vendor string. Compare against the AMD
    ; code.
    mov rax, QWORD dc_asm_ID_AMD
    cmp ecx, [rax]
    jne dc_asm_amd_trig_arg
    jmp dc_asm_intel_trig_arg

DC_ASM_WriteSqrtArg:
    ; Get the XMM number
    mov rax, QWORD dc_asm_index
    mov r8d, [rax]
    mov rax, 4 ; rax will function as the write index later.
    shl rsi, 2
    jz dc_asm_write_sqrt_zero_arg
    lea rdx, [(r8 * 8) + 0xF30F5146]
    bswap edx
    mov [rdi], edx
    mov cx, si
    mov [rdi+4], cl
    inc rax
    ret

dc_asm_write_sqrt_zero_arg:
    lea rcx, [(r8 * 8) + 0xF30F5106]
    bswap ecx
    mov [rdi], ecx
    ret

DC_ASM_WriteSqrtImm:
    mov ecx, 0xF30F5100
    jmp dc_asm_write_arg_immediate

DC_ASM_WriteAddImm:
    mov ecx, 0xF30F5800
    jmp dc_asm_write_arg_immediate

DC_ASM_WriteSubImm:
    mov ecx, 0xF30F5C00
    jmp dc_asm_write_arg_immediate

DC_ASM_WriteDivImm:
    mov ecx, 0xF30F5E00
    jmp dc_asm_write_arg_immediate

DC_ASM_WriteMulImm:
    mov ecx, 0xF30F5900
    ; jmp dc_asm_write_arg_immediate
    ; FALLTHROUGH

dc_asm_write_arg_immediate:
    ; Write mov [rax], IMM
    mov [rdi], WORD 0x00C7
    
    ; Get the immediate
    lea rax, [rsp-8]
    movss [rax], xmm0
    mov edx, [rax]
    mov [rdi+2], edx

    ; Get the XMM register.
    mov rax, QWORD dc_asm_index
    mov edx, [rax]
    shl edx, 2
    or cl, dl
    mov [rdi+6], ecx
    mov rax, 10
    ret

DC_ASM_WritePop:
    mov rax, QWORD dc_asm_index
    dec DWORD [rax]
    ret

DC_ASM_WriteRet:
    mov [rdi], BYTE 0xC3
    mov rax, QWORD dc_asm_index
    dec DWORD [rax]
    mov rax, 1
    ret

; void DC_ASM_Calculate(const void *addr, const float *args, float *result);
DC_ASM_Calculate:
    push rdx
    lea rax,[rsp-24]
    call rdi
    pop rdx
    movss [rdx], xmm0
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Intel trig functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; On Intel, using cvtss2sd/cvtsd2ss is about the slowest thing you can do, and
; fld is about the same speed on 32-bit values as 64-bit values. Therefor, it's
; best to just movss to the stack, then fld the DWORD.

; This is used for the sin/cos calculations.
; dx has the operation to use in the x87 registers.
; rsi has the argument index
dc_asm_intel_trig_arg:
    shl rsi, 2
    je dc_asm_intel_trig_zero_arg
    mov cx, si
    
    mov [rdi], WORD 0x46D9
    mov [rdi+2], cl
    mov rax, 3
    jmp dc_asm_intel_trig
    
dc_asm_intel_trig_zero_arg:
    mov [rdi], WORD 0x06D9
    mov rax, 2
    ; FALLTHROUGH

; This is used for the sin/cos calculations.
; dx has the operation to use in the x87 registers.
; rax has our current write offset.
dc_asm_intel_trig:
    ; fsin/fcos
    mov [rdi+rax], dx
    
    ; Write fstp DWORD [rax]
    mov [rdi+rax+2], WORD 0x18D9
    
    lea rdx, [(r8 * 8) + 0xF30F1000]
    mov [rdi+rax+4], edx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; AMD trig functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; It's slightly faster on AMD to use cvtss2sd and then write 64-bit doubles to
; the stack, then read those into the x87 registers than to write 32-bit floats
; to the stack then read those in to the x87 registers.
; This is COMPLETELY different on Intel, where fld is about the same speed for
; 32 and 64 bit values. The cvt is pretty slow on both, but the 32-bit fld is
; surprisingly slow on AMD, which is what changes the balance.

; This is used for the sin/cos calculations.
; dx has the operation to use in the x87 registers.
; rsi has the argument index
dc_asm_amd_trig_arg:
    xor rax, rax
    
    ; Convert the float arg to a double in XMM+1
    ; Write that to [rax]
    
    shl esi, 2
    je dc_asm_amd_trig_zero_arg
    
    mov cx, si
    ; cvtss2sd XMM+1, [rsi+arg]
    lea rsi, [(r8 * 8) + 0xF30F5A4E]
    bswap esi
    mov [rdi+rax], esi
    mov [rdi+rax+4], cl
    add rax, 5
    jmp dc_asm_amd_trig_arg_rejoin

dc_asm_amd_trig_zero_arg:
    ; cvtss2sd XMM+1, [rsi]
    lea rsi, [(r8 * 8) + 0xF30F5A0E]
    bswap esi
    mov [rdi+rax], esi
    add rax, 4
    
dc_asm_amd_trig_arg_rejoin:
    ; movsd [rax], XMM+1
    lea rcx, [(r8 * 8) + 0xF20F1108]
    bswap ecx
    mov [rdi+rax], ecx
    add rax, 4
    ; FALLTHROUGH

; This is used for the sin/cos calculations.
; dx has the operation to use in the x87 registers.
; rax has our current write offset.
dc_asm_amd_trig:
    ; fld QWORD [rax]
    mov [rdi+rax], WORD 0x00DD
    
    ; fsin/fcos
    mov [rdi+rax+2], dx
    
    ; fstp QWORD [rax]
    mov [rdi+rax+4], WORD 0x18DD
    
    ; cvtsd2ss XMM, [rax]
    lea rax, [(r8 * 8) + 0xF20F5A00]
    bswap eax
    mov [rdi+rax+6], eax
    add rax, 10
    ret

section .bss
    DC_ASM_pop_size: ; FALLTHROUGH
    dc_zero_memory: resd 1
    dc_asm_index: resd 1

section .data
    
    dc_asm_ID_AMD: db "cAMD"
    
    dc_asm_arithmetic_codes: db 0xC1,0xCA,0xD3,0xDC,0xE5,0xEE,0xF7
    DC_ASM_ret_size: dd 1
    DC_ASM_sin_size: ; FALLTHROUGH
    DC_ASM_cos_size: ; FALLTHROUGH
    DC_ASM_sqrt_size: dd 15
    DC_ASM_add_arg_size: ; FALLTHROUGH
    DC_ASM_sub_arg_size: ; FALLTHROUGH
    DC_ASM_div_arg_size: ; FALLTHROUGH
    DC_ASM_mul_arg_size: ; FALLTHROUGH
    DC_ASM_sqrt_arg_size: ; FALLTHROUGH
    DC_ASM_push_arg_size: dd 5
    DC_ASM_sin_arg_size: ; FALLTHROUGH
    DC_ASM_cos_arg_size: ; FALLTHROUGH
    DC_ASM_sin_imm_size: ; FALLTHROUGH
    DC_ASM_cos_imm_size: dd 43
    DC_ASM_add_imm_size: ; FALLTHROUGH
    DC_ASM_sub_imm_size: ; FALLTHROUGH
    DC_ASM_div_imm_size: ; FALLTHROUGH
    DC_ASM_mul_imm_size: ; FALLTHROUGH
    DC_ASM_sqrt_imm_size: ; FALLTHROUGH
    DC_ASM_immediate_size: dd 10
    DC_ASM_jmp_size: dd 13
    DC_ASM_add_size: ; FALLTHROUGH
    DC_ASM_sub_size: ; FALLTHROUGH
    DC_ASM_mul_size: ; FALLTHROUGH
    DC_ASM_div_size: dd 4

