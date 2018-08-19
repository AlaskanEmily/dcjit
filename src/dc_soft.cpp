// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "dc_backend.h"

#include <stack>
#include <vector>
#include <string>
#include <stdlib.h>

namespace DC {

typedef std::stack<float, std::vector<float> > Stack;

enum ExpressionType {
    eImmediate,
    eArgument,
    ePop
};

class Expression {
    ExpressionType m_type;
    union {
        float u_imm;
        unsigned short u_arg;
    } m_data;
    
    Expression(ExpressionType t)
      : m_type(t){}
    
public:
    
    // Needed for std::vector
    Expression(){}
    
    // Creates an immediate element
    static Expression Immediate(float i){
        Expression e = Expression(eImmediate);
        e.m_data.u_imm = i;
        return e;
    }
    
    // Creates an argument element
    static Expression Argument(unsigned short a){
        Expression e = Expression(eArgument);
        e.m_data.u_arg = a;
        return e;
    }
    
    // Creates a stack/pop element
    static Expression Pop(){
        return Expression(ePop);
    }
    
    float eval(Stack &stack, const float *const args) const {
        switch(m_type){
            case eImmediate:
                return m_data.u_imm;
            case ePop:
                {
                    const float r = stack.top();
                    stack.pop();
                    return r;
                }
            case eArgument:
                return args[m_data.u_arg];
        }
        abort();
    }
};

class Calculation {
    std::vector<Expression> m_expressions;
public:
    
    void PushImmediate(float i){
        m_expressions.push_back(Expression::Immediate(i));
    }
    
    void PushArgument(unsigned short a){
        m_expressions.push_back(Expression::Argument(a));
    }
    
    void PushPop(){
        m_expressions.push_back(Expression::Pop());
    }
    
    float calculate(const float *const args) const {
        Stack stack;
        for(std::vector<Expression>::const_iterator i = m_expressions.begin();
            i != m_expressions.end();
            i++){
            
            stack.push(i->eval(stack, args));
        }
        
        return stack.top();
    }
};

} // namespace DC

struct DC_X_Context {};

// Use inheritence to allow casting through the calculation finalization
struct DC_X_Calculation : public DC::Calculation {};
struct DC_X_CalculationBuilder : public DC_X_Calculation {};

DC_X_Context *DC_X_CreateContext(void){
    return new DC_X_Context;
}

void DC_X_FreeContext(DC_X_Context *ctx){
    delete ctx;
}

DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(DC_X_Context *ctx){
    (void)ctx;
    return new DC_X_CalculationBuilder();
}

void DC_X_BuildPushImmediate(DC_X_Context *ctx, DC_X_CalculationBuilder *bld, float value){
    (void)ctx;
    bld->PushImmediate(value);
}

void DC_X_BuildPushArg(DC_X_Context *ctx, DC_X_CalculationBuilder *bld, unsigned short arg_num){
    (void)ctx;
    bld->PushArgument(arg_num);
}

void DC_X_BuildPop(DC_X_Context *ctx, DC_X_CalculationBuilder *bld){
    (void)ctx;
    bld->PushPop();
}

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

float DC_X_Calculate(const DC_X_Calculation *calc, const float *args){
    return calc->calculate(args);
}
