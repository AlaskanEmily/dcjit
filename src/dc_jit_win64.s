; Copyright (c) 2018, Transnat Games
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.
;
; Wraps the real functions so that they can be called from the Win64 calling
; conventions.
;
; In some places we use `mov [rsp-8], r64`. This is because Win64 guarantees
; 32 bytes of "shadow space" on the stack. If we need to push two arguments,
; we can instead push into this shadow space to avoid two pushes and an
; `add rsp, 8` to keep the stack aligned to 16 bytes.

section .text
bits 64

%define DC_ASM_FunctionWin64(DC_NAME) DC_NAME %+ _Win64

%macro DC_ASM_FunctionWrapper 1
extern %1
global DC_ASM_FunctionWin64(%1)
%endmacro

%macro DC_ASM_SingleIntArgBody 1
    ;mov [rsp+16], rdi
    push rsi
    push rdi
    mov rdi, rcx
    call %1
    pop rdi
    pop rsi
    ;mov rdi, [rsp+16]
    ret
%endmacro

%macro DC_ASM_SingleIntArgFunc 1
DC_ASM_FunctionWrapper %1
DC_ASM_FunctionWin64(%1):
    DC_ASM_SingleIntArgBody %1
%endmacro

%macro DC_ASM_SingleIntSingleFloatArgFunc 1
DC_ASM_FunctionWrapper %1
DC_ASM_FunctionWin64(%1):
    movaps xmm0, xmm1
    DC_ASM_SingleIntArgBody %1
%endmacro

%macro DC_ASM_SingleIntSingleShortArgFunc 1
DC_ASM_FunctionWrapper %1
DC_ASM_FunctionWin64(%1):
;    mov [rsp+16], rdi
    push r12
    push rdi
    push rsi
    mov rdi, rcx
    movzx rsi, dx
    call %1
    pop rsi
    pop rdi
    pop r12
;    mov rdi, [rsp+16]
    ret
%endmacro

%macro DC_ASM_SingleIntSinglePointerArgFunc 1
DC_ASM_FunctionWrapper %1
DC_ASM_FunctionWin64(%1):
    mov [rsp+16], rdi
    push rsi
    mov rdi, rcx
    mov rsi, rdx
    call %1
    pop rsi
    mov rdi, [rsp+16]
    ret
%endmacro

DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_WriteImmediate
DC_ASM_SingleIntSinglePointerArgFunc DC_ASM_WriteJMP
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WritePushArg

DC_ASM_SingleIntArgFunc DC_ASM_WritePop
DC_ASM_SingleIntArgFunc DC_ASM_WriteRet

DC_ASM_SingleIntArgFunc DC_ASM_WriteAdd
DC_ASM_SingleIntArgFunc DC_ASM_WriteSub
DC_ASM_SingleIntArgFunc DC_ASM_WriteMul
DC_ASM_SingleIntArgFunc DC_ASM_WriteDiv

DC_ASM_SingleIntArgFunc DC_ASM_WriteSin
DC_ASM_SingleIntArgFunc DC_ASM_WriteCos
DC_ASM_SingleIntArgFunc DC_ASM_WriteSqrt

DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteAddArg
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteSubArg
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteMulArg
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteDivArg

DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteSinArg
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteCosArg
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_WriteSqrtArg

DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_WriteAddImm
DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_WriteSubImm
DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_WriteMulImm
DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_WriteDivImm

extern DC_ASM_Calculate
global DC_ASM_Calculate_Win64
DC_ASM_Calculate_Win64:
    mov [rsp+16], rdi
    push rsi
    mov rdi, rcx
    mov rsi, rdx
    mov rdx, r8
    call DC_ASM_Calculate
    pop rsi
    mov rdi, [rsp+16]
    ret

