/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dc_jit.h"

/*
 * Some JIT commands can be composed of multiple simpler commands. The reason
 * these commands exist is that they often can be implemented in a more
 * efficient manner when combined.
 *
 * This file defines all the composable commands in C to allow a backend to be
 * created with a bare minimum of work. It is also not unlikely that some 
 */

#define ARG_OP(NAME)\
unsigned DC_ASM_Write ## NAME ## Arg(void *dest,\
    unsigned short arg){\
    const unsigned s0 = DC_ASM_WritePushArg(dest, arg);\
    const unsigned s1 = DC_ASM_Write ## NAME(((unsigned char *)dest)+s0);\
    return s0 + s1;\
}

ARG_OP(Add)
ARG_OP(Sub)
ARG_OP(Mul)
ARG_OP(Div)
ARG_OP(Sin)
ARG_OP(Cos)
ARG_OP(Sqrt)
