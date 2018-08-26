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

%define DC_ASM_Function(DC_NAME) DC_ASM_Write %+ DC_NAME
%define DC_ASM_Function_Win64(DC_NAME) DC_ASM_Write %+ DC_NAME %+ _Win64

%macro DC_ASM_FunctionDecl 1
extern DC_ASM_Function(%1)
global DC_ASM_Function_Win64(%1)
%endmacro

%define DC_ASM_FunctionArg(DC_NAME) DC_NAME %+ Arg
%define DC_ASM_FunctionImm(DC_NAME) DC_NAME %+ Imm

%macro DC_ASM_SingleIntArgBody 1
    mov [rsp-8], rdi
    push rsi
    mov rdi, rcx
    call DC_ASM_Function(%1)
    pop rsi
    mov rdi, [rsp-8]
    ret
%endmacro

%macro DC_ASM_SingleIntArgFunc 1
DC_ASM_Function_Win64(%1):
    DC_ASM_SingleIntArgBody %1
%endmacro

%macro DC_ASM_SingleIntSingleFloatArgFunc 1
DC_ASM_Function_Win64(%1):
    movss xmm0, xmm1
    DC_ASM_SingleIntArgBody %1
%endmacro

%macro DC_ASM_SingleIntSingleShortArgFunc 1
DC_ASM_Function_Win64(%1):
    mov [rsp-8], rdi
    push rsi
    mov rdi, rcx
    mov rsi, rdx
    call DC_ASM_Function(%1)
    pop rsi
    mov rdi, [rsp-8]
    ret
%endmacro

%macro DC_ASM_ArithmeticFuncDecl 1
DC_ASM_FunctionDecl %1
DC_ASM_FunctionDecl DC_ASM_FunctionArg(%1)
DC_ASM_FunctionDecl DC_ASM_FunctionImm(%1)
DC_ASM_SingleIntArgFunc %1
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_FunctionArg(%1)
DC_ASM_SingleIntSingleFloatArgFunc DC_ASM_FunctionImm(%1)
%endmacro

%macro DC_ASM_TrigFuncDecl 1
DC_ASM_FunctionDecl %1
DC_ASM_FunctionDecl DC_ASM_FunctionArg(%1)
DC_ASM_SingleIntArgFunc %1
DC_ASM_SingleIntSingleShortArgFunc DC_ASM_FunctionArg(%1)
%endmacro

DC_ASM_ArithmeticFuncDecl Add
DC_ASM_ArithmeticFuncDecl Sub
DC_ASM_ArithmeticFuncDecl Mul
DC_ASM_ArithmeticFuncDecl Div
DC_ASM_TrigFuncDecl Sin
DC_ASM_TrigFuncDecl Cos
DC_ASM_TrigFuncDecl Sqrt

extern DC_ASM_WriteJMP
global DC_ASM_WriteJMP_Win64
DC_ASM_WriteJMP_Win64:
    mov [rsp+16], rdi
    push rsi
    mov rsi, rdx
    call DC_ASM_WriteJMP
    pop rsi
    mov rdi, [rsp+16]
    ret

extern DC_ASM_WritePushArg
global DC_ASM_WritePushArg_Win64
DC_ASM_WritePushArg_Win64:
    mov [rsp+16], rdi
    push rsi
    movzx rsi, dx
    mov rdi, rcx
    call DC_ASM_WriteJMP
    pop rsi
    mov rdi, [rsp+16]
    ret

extern DC_ASM_WriteImmediate
global DC_ASM_WriteImmediate_Win64
DC_ASM_WriteImmediate_Win64:
    mov [rsp+16], rdi
    push rsi
    mov rdi, rcx
    movss xmm0, xmm1
    call DC_ASM_WriteImmediate
    pop rsi
    mov rdi, [rsp+16]
    ret

DC_ASM_FunctionDecl JMP
DC_ASM_FunctionDecl PushArg
DC_ASM_FunctionDecl Immediate

extern DC_ASM_WritePop
global DC_ASM_WritePop_Win64
DC_ASM_WritePop_Win64:
    mov [rsp+16], rdi
    push rsi
    mov rdi, rcx
    call DC_ASM_WritePop
    pop rsi
    mov rdi, [rsp+16]
    ret

extern DC_ASM_WriteRet
global DC_ASM_WriteRet_Win64
DC_ASM_WriteRet_Win64:
    mov [rsp+16], rdi
    push rsi
    mov rdi, rcx
    call DC_ASM_WriteRet
    pop rsi
    mov rdi, [rsp+16]
    ret

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

