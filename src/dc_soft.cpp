// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "dc_backend.h"

#include <stack>
#include <vector>
#include <math.h>

namespace DC {

class Operation {
public:
    virtual void run(std::stack<float> &stack, const float *args) const = 0;
    virtual ~Operation(){}
};

class Pop : public Operation {
public:
    virtual void run(std::stack<float> &stack, const float *args) const {
        (void)args;
        stack.pop();
    }
};

class UnaryOp : public Operation {
public:
    virtual void run(std::stack<float> &stack, const float *args) const {
        (void)args;
        stack.top() = op(stack.top());
    }
    virtual float op(float a) const = 0;
};

class Sin : public UnaryOp {
public:
    virtual float op(float a) const {
        return static_cast<float>(sin(a));
    }
};

class Cos : public UnaryOp {
public:
    virtual float op(float a) const {
        return static_cast<float>(cos(a));
    }
};

class Sqrt : public UnaryOp {
public:
    virtual float op(float a) const {
        return static_cast<float>(sqrt(a));
    }
};

class BinaryOp : public Operation {
public:
    virtual void run(std::stack<float> &stack, const float *args) const {
        (void)args;
        const float a = stack.top();
        stack.pop();
        stack.top() = op(a, stack.top());
    }
    virtual float op(float a, float b) const = 0;
};

class Add : public BinaryOp {
public:
    virtual float op(float a, float b) const {
        return a + b;
    }
};

class Sub : public BinaryOp {
public:
    virtual float op(float a, float b) const {
        return a - b;
    }
};

class Div : public BinaryOp {
public:
    virtual float op(float a, float b) const {
        return a / b;
    }
};

class Mul : public BinaryOp {
public:
    virtual float op(float a, float b) const {
        return a * b;
    }
};

class Term : public Operation {
public:
    virtual float evaluate(const float *args) const = 0;
    virtual void run(std::stack<float> &stack, const float *args) const {
        stack.push(evaluate(args));
    }
};

class Argument : public Term {
    unsigned short m_arg_num;
public:
    Argument(unsigned short arg_num)
      : m_arg_num(arg_num){}
    
    virtual float evaluate(const float *args) const {
        return args[m_arg_num];
    }
};

class Immediate : public Term {
    float m_immediate;
public:
    Immediate(float immediate)
      : m_immediate(immediate){}
    
    virtual float evaluate(const float *args) const {
        (void)args;
        return m_immediate;
    }
};

} // namespace DC

struct DC_X_Context{};

struct DC_X_Calculation {
    virtual ~DC_X_Calculation() {}
    virtual float run(const float *args) const = 0;
};

struct DC_X_CalculationBuilder : public DC_X_Calculation{
private:
    typedef std::vector<DC::Operation*> OperationVector;
    typedef OperationVector::const_iterator iterator;
    
    OperationVector m_operations;
public:
    virtual ~DC_X_CalculationBuilder() {
        for(iterator i = m_operations.begin(); i != m_operations.end(); i++){
            delete *i;
        }
    }
    float run(const float *args) const {
        std::stack<float> stack;
        for(iterator i = m_operations.begin();
            i != m_operations.end();
            i++){
            const DC::Operation &op = **i;
            op.run(stack, args);
        }
        return stack.top();
    }
    void push(DC::Operation *op){
        m_operations.push_back(op);
    }
    
    template<typename T>
    void push(){
        DC::Operation *const op = new T();
        m_operations.push_back(op);
    }
};

DC_X_Context *DC_X_CreateContext(void){
    return new DC_X_Context();
}

void DC_X_FreeContext(DC_X_Context *ctx){
    delete ctx;
}

DC_X_CalculationBuilder *DC_X_CreateCalculationBuilder(
    struct DC_X_Context *ctx){
    (void)ctx;
    return new DC_X_CalculationBuilder();
}

void DC_X_BuildPushImmediate(DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    float value){
    (void)ctx;
    bld->push(new DC::Immediate(value));
}

void DC_X_BuildPushArg(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    unsigned short arg_num){
    (void)ctx;
    bld->push(new DC::Argument(arg_num));
}

#define DC_X_BUILD(NAME)\
void DC_X_Build ## NAME(struct DC_X_Context *ctx,\
    struct DC_X_CalculationBuilder *bld){\
    (void)ctx;\
    bld->push<DC::NAME>();\
}\
void DC_X_Build ## NAME ## Arg(struct DC_X_Context *ctx,\
    struct DC_X_CalculationBuilder *bld,\
    unsigned short arg_num){\
    (void)ctx;\
    bld->push(new DC::Argument(arg_num));\
    bld->push<DC::NAME>();\
}

DC_X_BUILD(Add)
DC_X_BUILD(Sub)
DC_X_BUILD(Div)
DC_X_BUILD(Mul)
DC_X_BUILD(Sqrt)
DC_X_BUILD(Sin)
DC_X_BUILD(Cos)

#define DC_X_BUILD_IMM(NAME)\
void DC_X_Build ## NAME ## Imm(struct DC_X_Context *ctx,\
    struct DC_X_CalculationBuilder *bld,\
    float imm){\
    (void)ctx;\
    bld->push(new DC::Immediate(imm));\
    bld->push<DC::NAME>();\
}

DC_X_BUILD_IMM(Add)
DC_X_BUILD_IMM(Sub)
DC_X_BUILD_IMM(Div)
DC_X_BUILD_IMM(Mul)

void DC_X_AbandonCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld){
    (void)ctx;
    delete bld;
}

struct DC_X_Calculation *DC_X_FinalizeCalculation(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld){
    (void)ctx;
    return bld;
}

void DC_X_Free(struct DC_X_Context *ctx, struct DC_X_Calculation *calc){
    (void)ctx;
    delete calc;
}

float DC_X_Calculate(const struct DC_X_Calculation *calc, const float *args){
    return calc->run(args);
}
