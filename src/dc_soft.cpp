// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "dc_bytecode.hpp"
#include "dc_backend.h"

#include <vector>
#include <math.h>
#include <assert.h>

// Software backend
// The build instructions assembly bytecode.
// Running interprets the bytecode format defined in dc_bytecode.hpp

struct DC_X_Context {};

// Inheriting like this allows us to implement the Finalize method as passthrough.
struct DC_X_Calculation : public DC::Bytecode::Bytecode {

};

struct DC_X_CalculationBuilder : public DC_X_Calculation {

};

DC_X_Context *DC_X_CreateContext(void){
    return new DC_X_Context;
}

void DC_X_FreeContext(struct DC_X_Context *ctx){
    delete ctx;
}

DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(DC_X_Context *ctx){
    (void)ctx;
    return new DC_X_CalculationBuilder;
}

void DC_X_BuildPushImmediate(DC_X_Context *ctx, DC_X_CalculationBuilder *bld, float value){
    (void)ctx;
    bld->writeImmediate(value);
}

void DC_X_BuildPushArg(DC_X_Context *ctx, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    (void)ctx;
    bld->writeArgument(arg_num);
}

#define DC_SOFT_UNOP(NAME)\
void DC_X_Build ## NAME(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){\
    (void)ctx; \
    bld->writeUnary<DC::Bytecode::e ## NAME>(); \
}\
void DC_X_Build ## NAME ## Arg(DC_X_Context *ctx, \
    DC_X_CalculationBuilder *bld, unsigned short arg_num){\
    (void)ctx; \
    bld->writeUnaryArgument<DC::Bytecode::e ## NAME>(arg_num); \
}

#define DC_SOFT_BINOP(NAME)\
void DC_X_Build ## NAME(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){\
    (void)ctx; \
    bld->writeBinary<DC::Bytecode::e ## NAME>(); \
} \
void DC_X_Build ## NAME ## Arg(DC_X_Context *ctx, \
    DC_X_CalculationBuilder *bld, unsigned short arg_num){\
    (void)ctx; \
    bld->writeBinaryArgument<DC::Bytecode::e ## NAME>(arg_num); \
} \
void DC_X_Build ## NAME ## Imm(DC_X_Context *ctx, \
    DC_X_CalculationBuilder *bld, float value){\
    (void)ctx; \
    bld->writeBinaryImmediate<DC::Bytecode::e ## NAME>(value); \
}

void DC_X_BuildPop(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){\
    (void)ctx;
    bld->writeUnary<DC::Bytecode::ePop>();
}

DC_SOFT_UNOP(Sin)
DC_SOFT_UNOP(Cos)
DC_SOFT_UNOP(Sqrt)

DC_SOFT_BINOP(Add)
DC_SOFT_BINOP(Sub)
DC_SOFT_BINOP(Mul)
DC_SOFT_BINOP(Div)

void DC_X_AbandonCalculation(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){
    (void)ctx;
    delete bld;
}

DC_X_Calculation *DC_X_FinalizeCalculation(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){
    (void)ctx;
    return bld;
}

void DC_X_Free(DC_X_Context *ctx, DC_X_Calculation *calc){
    (void)ctx;
    delete calc;
}

float DC_X_Calculate(const struct DC_X_Calculation *calc, const float *args){
    DC::Bytecode::Bytecode::iterator iter = calc->begin(), end = calc->end();
    std::vector<float> stack;
    stack.reserve(16); // Pretty reasonable guess
    while(iter != end){
        switch(iter.opType()){
            case DC::Bytecode::eImmediate:
                // Push an immediate onto the stack
                stack.push_back(iter.readImmediate());
                continue;
            case DC::Bytecode::eArgument:
                // Push an argument onto the stack
                {
                    const unsigned short arg_num = iter.readArgument();
                    const float value = args[arg_num];
                    stack.push_back(value);
                }
                continue;
            case DC::Bytecode::eUnary:
                // Get the value to operate on.
                // All unary ops work on the top value of the stack.
                {
                    assert(!stack.empty());
                    const float value = stack.back();
                    switch(iter.readUnaryOp()){
                        case DC::Bytecode::eSin:
                            stack.back() = sin(value);
                            continue;
                        case DC::Bytecode::eCos:
                            stack.back() = cos(value);
                            continue;
                        case DC::Bytecode::eSqrt:
                            stack.back() = sqrt(value);
                            continue;
                        case DC::Bytecode::ePop:
                            stack.pop_back();
                            continue;
                    }
                }
                assert(NULL == "Invalid unary op.");
                continue;
            case DC::Bytecode::eBinary:
                assert(stack.size() >= 2);
                {
                    const float value = stack.back();
                    stack.pop_back();
                    switch(iter.readBinaryOp()){
                        case DC::Bytecode::eAdd:
                            stack.back() += value;
                            continue;
                        case DC::Bytecode::eSub:
                            stack.back() -= value;
                            continue;
                        case DC::Bytecode::eMul:
                            stack.back() *= value;
                            continue;
                        case DC::Bytecode::eDiv:
                            stack.back() /= value;
                            continue;
                    }
                }
                assert(NULL == "Invalid binary op.");
                continue;
        }
        assert(NULL == "Invalid op type.");
        continue;
    }
    
    assert(stack.size() == 1);
    return stack.back();
}
