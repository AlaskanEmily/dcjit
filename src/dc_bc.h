/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBDCJIT_DC_BC_H
#define LIBDCJIT_DC_BC_H
#pragma once

/* Interface for generating bytecode from C.
 *
 * This is used for the root finder API, as well as the interpreter.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct DC_Bytecode;

/* Note that in the dummied backend, this will return NULL. */
struct DC_Bytecode *DC_BC_CreateBytecode(void);

void DC_BC_FreeBytecode(struct DC_Bytecode *bc);

void DC_BC_BuildPushImmediate(struct DC_Bytecode *bc, float value);

void DC_BC_BuildPushArg(struct DC_Bytecode *bc, unsigned short arg_num);

void DC_BC_BuildAdd(struct DC_Bytecode *bc);

void DC_BC_BuildSub(struct DC_Bytecode *bc);

void DC_BC_BuildMul(struct DC_Bytecode *bc);

void DC_BC_BuildDiv(struct DC_Bytecode *bc);

void DC_BC_BuildPop(struct DC_Bytecode *bc);

void DC_BC_BuildSin(struct DC_Bytecode *bc);

void DC_BC_BuildCos(struct DC_Bytecode *bc);

void DC_BC_BuildSqrt(struct DC_Bytecode *bc);

void DC_BC_BuildAddArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildSubArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildMulArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildDivArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildSinArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildCosArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildSqrtArg(struct DC_Bytecode *bc, unsigned short arg);

void DC_BC_BuildAddImm(struct DC_Bytecode *bc, float imm);

void DC_BC_BuildSubImm(struct DC_Bytecode *bc, float imm);

void DC_BC_BuildMulImm(struct DC_Bytecode *bc, float imm);

void DC_BC_BuildDivImm(struct DC_Bytecode *bc, float imm);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_BYTECODE_HPP */
