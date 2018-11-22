// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "dc_bc.h"
#include "dc_bytecode.hpp"

// Interface for the bytecode object to be called from C.
//
// Note that there is actually no struct DC_Bytecode, we simply C-cast it to the actual object.
// This is because MSVC will complain about extern "C"'ing a C++ object, and there is no practical
// difference between doing "struct DC_Bytecode : public DC::Bytecode::Bytecode" and just
// C-casting here.

struct DC_Bytecode *DC_BC_CreateBytecode(void){
    return (DC_Bytecode *)(new DC::Bytecode::Bytecode);
}

void DC_BC_FreeBytecode(struct DC_Bytecode *bc){
    delete (DC::Bytecode::Bytecode*)bc;
}

void DC_BC_BuildPushImmediate(struct DC_Bytecode *bc, float value){
    ((DC::Bytecode::Bytecode*)bc)->writeImmediate(value);
}

void DC_BC_BuildPushArg(struct DC_Bytecode *bc, unsigned short arg_num){
    ((DC::Bytecode::Bytecode*)bc)->writeArgument(arg_num);
}

void DC_BC_BuildPop(struct DC_Bytecode *bc){
    ((DC::Bytecode::Bytecode*)bc)->writeUnary<DC::Bytecode::ePop>();
}

#define DC_BC_BINARY_OP(NAME) \
void DC_BC_Build ## NAME(struct DC_Bytecode *bc){ \
    ((DC::Bytecode::Bytecode*)bc)->writeBinary<DC::Bytecode::e ## NAME>(); \
} \
void DC_BC_Build ## NAME ## Arg(struct DC_Bytecode *bc, unsigned short arg){ \
    ((DC::Bytecode::Bytecode*)bc)->writeBinaryArgument<DC::Bytecode::e ## NAME>(arg); \
} \
void DC_BC_Build ## NAME ## Imm(struct DC_Bytecode *bc, float imm){ \
    ((DC::Bytecode::Bytecode*)bc)->writeBinaryImmediate<DC::Bytecode::e ## NAME>(imm); \
}

DC_BC_BINARY_OP(Add)
DC_BC_BINARY_OP(Sub)
DC_BC_BINARY_OP(Div)
DC_BC_BINARY_OP(Mul)

#define DC_BC_UNARY_OP(NAME) \
void DC_BC_Build ## NAME(struct DC_Bytecode *bc){ \
    ((DC::Bytecode::Bytecode*)bc)->writeUnary<DC::Bytecode::e ## NAME>(); \
} \
void DC_BC_Build ## NAME ## Arg(struct DC_Bytecode *bc, unsigned short arg){ \
    ((DC::Bytecode::Bytecode*)bc)->writeUnaryArgument<DC::Bytecode::e ## NAME>(arg); \
}

DC_BC_UNARY_OP(Sin)
DC_BC_UNARY_OP(Cos)
DC_BC_UNARY_OP(Sqrt)
