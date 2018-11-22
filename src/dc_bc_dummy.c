/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include "dc_bc.h"

#include <stdlib.h>

/* Dummy implementation of bytecode for when the library is compiled without
 * the interpreter or root-finding support (or when C++ is not available).
 */

struct DC_Bytecode *DC_BC_CreateBytecode(void){ return NULL; }

void DC_BC_FreeBytecode(struct DC_Bytecode *bc) { (void)bc; }

void DC_BC_BuildPushImmediate(struct DC_Bytecode *bc, float value) {
    (void)bc; (void)value;
}

void DC_BC_BuildPushArg(struct DC_Bytecode *bc, unsigned short arg_num) {
    (void)bc; (void)arg_num;
}

void DC_BC_BuildPop(struct DC_Bytecode *bc) { (void)bc; }

#define DC_BC_UNOP(NAME)\
void DC_BC_Build ## NAME (struct DC_Bytecode *bc) { (void)bc; } \
void DC_BC_Build ## NAME ## Arg(struct DC_Bytecode *bc, unsigned short a) { \
    (void)bc; (void)a; \
}

#define DC_BC_BINOP(NAME)\
DC_BC_UNOP(NAME) \
void DC_BC_Build ## NAME ## Imm(struct DC_Bytecode *bc, float x) { \
    (void)bc; (void)x; \
}

DC_BC_BINOP(Add)
DC_BC_BINOP(Sub)
DC_BC_BINOP(Mul)
DC_BC_BINOP(Div)

DC_BC_UNOP(Cos)
DC_BC_UNOP(Sin)
DC_BC_UNOP(Sqrt)
