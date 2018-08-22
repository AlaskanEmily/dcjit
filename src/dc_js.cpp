// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "dc_backend.h"

#include <emscripten.h>

struct DC_X_Calculation{
    unsigned js_function_number;
    unsigned num_args;
};

struct DC_X_CalculationBuilder{
    unsigned js_string_number;
    unsigned num_args;
};

DC_X_Context *DC_X_CreateContext(void){
    return ((DC_X_Context*)8);
}

void DC_X_FreeContext(DC_X_Context *){}

DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(DC_X_Context *){
    const unsigned string_num = EM_ASM_INT("DC_JS_CreateCalculationBuilder()", 0);
    return new DC_X_CalculationBuilder{string_num, 0};
}

void DC_X_BuildPushImmediate(DC_X_Context *, DC_X_CalculationBuilder *bld, float value){
    const double d = static_cast<double>(value);
    EM_ASM("DC_JS_BuildPushImmediate($0, $1)", bld->js_string_number, d);
}

void DC_X_BuildPushArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildPushArg($0, $1)", bld->js_string_number, i);
}

void DC_X_BuildAdd(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildOperator($0, '+')", bld->js_string_number);
}

void DC_X_BuildSub(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildOperator($0, '-')", bld->js_string_number);
}

void DC_X_BuildMul(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildOperator($0, '*')", bld->js_string_number);
}

void DC_X_BuildDiv(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildOperator($0, '/')", bld->js_string_number);
}

void DC_X_BuildPop(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildPop($0)", bld->js_string_number);
}

void DC_X_BuildSin(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildMathBuiltin($0, 'sin')", bld->js_string_number);
}

void DC_X_BuildCos(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildMathBuiltin($0, 'cos')", bld->js_string_number);
}

void DC_X_BuildSqrt(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_BuildMathBuiltin($0, 'sqrt')", bld->js_string_number);
}

void DC_X_AbandonCalculation(DC_X_Context *, DC_X_CalculationBuilder *bld){
    EM_ASM("DC_JS_AbandonCalculation($0)", bld->js_string_number);
}

void DC_X_BuildAddArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildOperatorArg($0, '+', $1)", bld->js_string_number, i);
}

void DC_X_BuildSubArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildOperatorArg($0, '-', $1)", bld->js_string_number, i);
}

void DC_X_BuildMulArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildOperatorArg($0, '*', $1)", bld->js_string_number, i);
}

void DC_X_BuildDivArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildOperatorArg($0, '/', $1)", bld->js_string_number, i);
}

void DC_X_BuildSinArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildMathBuiltinArg($0, 'sin', $1)", bld->js_string_number, i);
}

void DC_X_BuildCosArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildMathBuiltinArg($0, 'cos', $1)", bld->js_string_number, i);
}

void DC_X_BuildSqrtArg(DC_X_Context *, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    const int i = static_cast<int>(arg_num);
    if(arg_num > bld->num_args)
        bld->num_args = arg_num;
    EM_ASM("DC_JS_BuildMathBuiltinArg($0, 'sqrt', $1)", bld->js_string_number, i);
}

void DC_X_BuildAddImm(DC_X_Context *, DC_X_CalculationBuilder *bld, float value){
    const double d = static_cast<double>(value);
    EM_ASM("DC_JS_BuildOperatorImm($0, '+', $1)", bld->js_string_number, d);
}


void DC_X_BuildSubImm(DC_X_Context *, DC_X_CalculationBuilder *bld, float value){
    const double d = static_cast<double>(value);
    EM_ASM("DC_JS_BuildOperatorImm($0, '-', $1)", bld->js_string_number, d);
}

void DC_X_BuildMulImm(DC_X_Context *, DC_X_CalculationBuilder *bld, float value){
    const double d = static_cast<double>(value);
    EM_ASM("DC_JS_BuildOperatorImm($0, '*', $1)", bld->js_string_number, d);
}

void DC_X_BuildDivImm(DC_X_Context *, DC_X_CalculationBuilder *bld, float value){
    const double d = static_cast<double>(value);
    EM_ASM("DC_JS_BuildOperatorImm($0, '/', $1)", bld->js_string_number, d);
}

DC_X_Calculation *DC_X_FinalizeCalculation(DC_X_Context *, DC_X_CalculationBuilder *bld){
    const unsigned js_function_number = EM_ASM_INT("DC_JS_FinalizeCalculation($0)", bld->js_string_number);
    const unsigned num_args = bld->num_args;
    delete bld;
    return new DC_X_Calculation{js_function_number, num_args};
}

void DC_X_Free(DC_X_Context *, DC_X_Calculation *calc){
   EM_ASM("DC_JS_Free($0)", calc->js_function_number);
}

float DC_X_Calculate(const struct DC_X_Calculation *calc, const float *args){
    EM_ASM("DC_JS_InitArgs()", 0);
    for(unsigned i = 0; i < calc->num_args; i++)
        EM_ASM("DC_JS_AppendArg($0)", static_cast<double>(args[i]));
    return EM_ASM_DOUBLE("DC_JS_Calculate($0)", static_cast<int>(calc->js_function_number));
}
