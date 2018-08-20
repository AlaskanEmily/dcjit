/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBDCJIT_DC_BACKEND_H
#define LIBDCJIT_DC_BACKEND_H
#pragma once

/*
 * Backend interface for libdcjit. The JIT, as well a software C++ version for
 * new/unsupported CPUs, implement this interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct DC_X_Context;
struct DC_X_Calculation;
struct DC_X_CalculationBuilder;

struct DC_X_Context *DC_X_CreateContext(void);
void DC_X_FreeContext(struct DC_X_Context *ctx);

struct DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(
    struct DC_X_Context *ctx);

void DC_X_BuildPushImmediate(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float value);

void DC_X_BuildPushArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg_num);

void DC_X_BuildAdd(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildSub(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildMul(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildDiv(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildPop(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildSin(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildCos(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildSqrt(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_AbandonCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_BuildAddArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildSubArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildMulArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildDivArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildSinArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildCosArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildSqrtArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg);

void DC_X_BuildAddImm(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float imm);

void DC_X_BuildSubImm(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float imm);

void DC_X_BuildMulImm(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float imm);

void DC_X_BuildDivImm(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float imm);

struct DC_X_Calculation *DC_X_FinalizeCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld);

void DC_X_Free(struct DC_X_Context *ctx, struct DC_X_Calculation *calc);

float DC_X_Calculate(const struct DC_X_Calculation *calc, const float *args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_H */
