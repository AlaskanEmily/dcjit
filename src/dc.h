﻿/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBDCJIT_DC_H
#define LIBDCJIT_DC_H

/*
 * The public API for the libdcjit library.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct DC_Context;
struct DC_Calculation;

struct DC_Context *DC_CreateContext(void);
void DC_FreeContext(struct DC_Context *ctx);

/* Returns the new calculation, or NULL if an error has occured. In that case,
 * out_error includes the error, which must have DC_FreeError() called on it
 * to free it.
 */
struct DC_Calculation *DC_Compile(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error);

void DC_FreeError(const char *error);

void DC_Free(struct DC_Context *ctx, struct DC_Calculation *);

float DC_Calculate(const struct DC_Calculation *, const float *args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_H */
