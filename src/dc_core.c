/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This file contains the DC language parser. It calls the DC_X functions to
 * actually compile the code (or write the AST if the interpreter backend is
 * being used).
 */

#include "dc.h"
#include "dc_backend.h"

/* needed for strncpy on some systems */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

/* Used for snprintf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DC_OPTIMIZE
#define DC_OPTIMIZE 1
#endif

#ifndef DC_OPTIMIZE_FETCH
#define DC_OPTIMIZE_FETCH DC_OPTIMIZE
#endif

#ifndef DC_OPTIMIZE_INTRINSIC
#define DC_OPTIMIZE_INTRINSIC DC_OPTIMIZE
#endif

#ifdef _MSC_VER
    #define DC_STRNCPY(DST, LEN, TXT) strncpy_s((DST), (LEN),  (TXT), _TRUNCATE)
#else
    #define DC_STRNCPY(DST, LEN, TXT) do {\
        char *const DC_STRNCPY_dst = (DST);\
        const long DC_STRNCPY_len = (LEN);\
        strncpy(DC_STRNCPY_dst, (TXT), DC_STRNCPY_len);\
        DC_STRNCPY_dst[DC_STRNCPY_len-1] = '\0';\
    }while(0)
#endif

/*
 * Parsing overview:
 *
 * DCJIT uses a fairly basic recursive descent parser. The ParseOperation
 * struct defines operators, and the language has two levels of operator
 * precedence. These are defined by parse_mul_ops and parse_add_ops, which use
 * the parse_generic function and specific data for the operators.
 *
 * All values encountered (parsed in parse_term) are considered "pushed",
 * "arguments", or "immediates".
 *
 * Pushed values have already had code generated that pushes the value onto the
 * stack.
 *
 * Argument values are indicated using their index into the argument list that
 * will be passed when the calculation is run. These will usually have to be
 * pushed, or used with an operator callback that can accept an argument index.
 *
 * Immediate values are constant expression values. These are propogated
 * upwards in the parser, and using callbacks on the ParseOperation all
 * subexpressions that consist solely of constant values are fully calculated
 * at runtime.
 *
 * TODO: Adding transitive properties (a+b+c = a+c+b) and identity values
 * (a = a+0, a = a*1.0) could allow even more aggressive constant expression
 * evaluation.
 *
 * How values are actually written for each operation is implemented in
 * parse_generic. Much of the code exists to handle the different value types,
 * which is mostly there for optimization.
 *
 * There are some terms which are "builtins". These consist of a function name
 * and a subexpression, such as "sin(<expression)". They are parsed as a single
 * term, and if possible an immediate result is returned. Otherwise, this will
 * result in a pushed value.
 */

/* This is the type of result of parse_term. */
enum TermResultType {
    eTermImmediate,
    eTermArgument,
    eTermPushed,
    eTermSyntaxError,
    eTermInvalidArgNumber,
    eTermInvalidArgName
};

/* The output of parse_term. The active member (if any) is indicated by the
 * type, in a TermResultType
 */
union TermType {
    double immediate;
    unsigned short argument;
};

/* Typedef for operations to calculate immediate results. */
typedef double(*arithmetic_operation)(double, double);

/* The build_push_* functions are the callbacks to perform codegen for an
 * arithmetic/trig operation.
 */

/* Typedef for operations to calculate results on fully pushed values. */
typedef void (*build_push_operation)(struct DC_X_Context*,
    struct DC_X_CalculationBuilder*);
/* Typedef for operations to calculate results with one pushed value and one
 * argument. */
typedef void (*build_push_arg_operation)(struct DC_X_Context*,
    struct DC_X_CalculationBuilder*,
    unsigned short);
/* Typedef for operations to calculate results with one pushed value and one
 * immediate value. Note that if both values are immediate, the
 * arithmetic_operation callback is used and the immediate result is propagated
 * upward through the parser. */
typedef void (*build_push_imm_operation)(struct DC_X_Context*,
    struct DC_X_CalculationBuilder*,
    float);

/* Immediate operation to add. */
static double arithmetic_operation_add(double a, double b) { return a + b; }
/* Immediate operation to subtract. */
static double arithmetic_operation_sub(double a, double b) { return a - b; }
/* Immediate operation to multiply. */
static double arithmetic_operation_mul(double a, double b) { return a * b; }
/* Immediate operation to divide. */
static double arithmetic_operation_div(double a, double b) { return a / b; }
/* Immediate operation to calculate sine. */
static double arithmetic_operation_sin(double a){ return sin(a); }
/* Immediate operation to calculate cosine. */
static double arithmetic_operation_cos(double a){ return cos(a); }
/* Immediate operation to calculate square root. */
static double arithmetic_operation_sqrt(double a){ return sqrt(a); }

/* Defines an operator in the language. */
struct ParseOperation {
    char operator_char; /* Operator character. */
    /* Callback for this operator with two immediate values. */
    arithmetic_operation immediate_op;
    /* Callback for this operator with two pushed values. */
    build_push_operation build_op;
    /* Callback for this operator with one pushed value and one argument. */
    build_push_arg_operation build_arg_op;
    build_push_imm_operation build_imm_op;
};

#define DC_BUILD_OPS(NAME)\
    DC_X_Build ## NAME, DC_X_Build ## NAME ## Arg, DC_X_Build ## NAME ## Imm

/* ParseOperation data for mul_ops 
 * TODO: Remainder will go here.
 */
#define DC_NUM_MUL_OPS 2
static const struct ParseOperation dc_mul_ops[DC_NUM_MUL_OPS] = {
    {'*', arithmetic_operation_mul, DC_BUILD_OPS(Mul)},
    {'/', arithmetic_operation_div, DC_BUILD_OPS(Div)}
};

/* ParseOperation data for add_ops */
#define DC_NUM_ADD_OPS 2
static const struct ParseOperation dc_add_ops[DC_NUM_ADD_OPS] = {
    {'+', arithmetic_operation_add, DC_BUILD_OPS(Add)},
    {'-', arithmetic_operation_sub, DC_BUILD_OPS(Sub)}
};

/* Unary operation typedef. This is only used in the "builtin" terms. */
typedef double(*unary_operation)(double);
typedef enum TermResultType(*parser_callback)(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term);

struct DC_Context *DC_CreateContext(void){
    return (struct DC_Context *)DC_X_CreateContext();
}

void DC_FreeContext(struct DC_Context *ctx){
    DC_X_FreeContext((struct DC_X_Context *)ctx);
}

/* Skips whitespace. */
static const char *skip_whitespace(const char *source){
skip_whitespace_next_char:
    {
        const int c = *source;
        if(c == ' ' || c == '\t'){
            source++;
            goto skip_whitespace_next_char;
        }
    }
    
    return source;
}

/* Parses an integer.
 * 
 * This is used for argument numbers, and to parse the whole and decimal parts
 * individually of floating point numbers. */
static unsigned long parse_integer(const char **const source_ptr){
    unsigned long val = 0;
    const char *source = *source_ptr;
    
next_digit:
    {
        int c = *source;
        if(c == '0'){
            val *= 10;
            /* Pass */
        }
        else if(c > '0' && c <= '9'){
            val *= 10;
            val += c - '0';
        }
        else{
            goto parse_done;
        }
        
        source++;
        goto next_digit;
    }
parse_done:
    
    source_ptr[0] = source;
    return val;
}

/* Parses a double-precision floating point number. */
static double parse_double(const char **const source_ptr){
    int negate = 0;
    const char *source = *source_ptr;
    switch(*source){
        case '-':
            negate = 1; /* FALLTHROUGH */
        case '+':
            source++;
    }
    {
        const unsigned long numerator = parse_integer(&source);
        if(*source == '.'){
            const char *start = ++source;
            unsigned divider = 1;
            const unsigned long fraction_int = parse_integer(&source);
            
            source_ptr[0] = source;
            while(start++ != source)
                divider *= 10;
            
            {
                const double fraction = ((double)fraction_int) / ((double)divider);
                const double value = ((double)numerator) + fraction;
                return (double)(negate ? (-value) : value);
            }
        }
        else{
            const double value = (double)numerator;
            source_ptr[0] = source;
            return negate ? (-value) : value;
        }
    }
}

/* Data for terms. */
union TermResult {
    union TermType term;
    struct {
        const char *string;
        unsigned length;
    } str_error;
    long int_error;
};

/* This is the general entry point to parse an expression */
static enum TermResultType parse_add_ops(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term);

/* Parses a value. This can be a literal, or an argument name or number.
 * Does not use the same data format as the other parsing functions, as the
 * caller will need to make decisions about what to with the result depending
 * on the operation.
 */
static enum TermResultType parse_value(const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermResult *out_result){
    
    const char *source = *source_ptr;
    
#define DC_TERM_END(TYPE) (source_ptr[0] = source, (TYPE))

#define DC_TERM_FAIL_INTEGER(TYPE, VALUE)\
    (out_result->int_error = (VALUE),DC_TERM_END(TYPE))

#define DC_TERM_FAIL_STRING(TYPE, VALUE, LENGTH) (\
    out_result->str_error.string = (VALUE),\
    out_result->str_error.length = (LENGTH),\
    DC_TERM_END(TYPE)\
    )

#define DC_TERM_SUCCCESS(TYPE, MEMBER, VALUE) (\
    out_result->term.MEMBER = (VALUE),\
    source_ptr[0] = source,\
    DC_TERM_END(TYPE)\
    )

#define DC_TERM_SUCCESS_ARG(VALUE) DC_TERM_SUCCCESS(eTermArgument, argument, (VALUE))
#define DC_TERM_SUCCESS_IMM(VALUE) DC_TERM_SUCCCESS(eTermImmediate, immediate, (VALUE))

    switch(*source){
        case '$':
            source++;
            {
                const unsigned long arg_num = parse_integer(&source);
                if(arg_num < 0x10000 && arg_num <= num_args)
                    return DC_TERM_SUCCESS_ARG((unsigned short)arg_num);
                else
                    return DC_TERM_FAIL_INTEGER(eTermInvalidArgNumber, arg_num);
            }
        case '-':
        case '+':
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return DC_TERM_SUCCESS_IMM(parse_double(&source));
        default:
            /* Parse an arg name */
            {
                const char *const arg_name_start = source;
                unsigned arg_num = 0, arg_name_size = 0;
            next_char:
                {
                    const int c = source[arg_name_size];
                    if(c == '_' ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c & 0x80)){
                        arg_name_size++;
                        goto next_char;
                    }
                }
            
            source += arg_name_size;
            
            next_arg_name:
                if(arg_num == num_args){
                    return DC_TERM_FAIL_STRING(eTermInvalidArgName,
                        arg_name_start,
                        arg_name_size);
                }
                else{
                    const char *const arg = arg_names[arg_num];
                    if(strncmp(arg_name_start, arg, arg_name_size) == 0 &&
                        arg[arg_name_size] == 0){
                        return DC_TERM_SUCCESS_ARG(arg_num);
                    }
                    else{
                        arg_num++;
                        goto next_arg_name;
                    }
                }
            }
    }
}

/* Parses a parenthesized expression. This will use the parse_add_ops function
 * to parse the inner statement.
 */
static enum TermResultType parse_parens(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term){
    
    if(**source_ptr == '('){
        const char *source = skip_whitespace(source_ptr[0]+1);
        const enum TermResultType type = parse_add_ops(ctx,
            bld,
            error_text,
            &source,
            num_args,
            arg_names,
            out_term);
        source = skip_whitespace(source);
        if(type == eTermImmediate || type == eTermArgument || type == eTermPushed){
            if(*source++ != ')'){
                DC_STRNCPY(error_text, 0xFF, "Expected )");
                return eTermSyntaxError;
            }
            source_ptr[0] = source;
        }
        return type;
    }
    else{
        DC_STRNCPY(error_text, 0xFF, "Expected (");
        return eTermSyntaxError;
    }
}

/* Parses a builtin, which is a parenthesized expression and a unary operation
 * to perform on that operation or to push to the JIT. */
static enum TermResultType builtin(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term,
    unary_operation immediate_operation,
    build_push_operation operation){
    
    /* Builtins are: <atom> '(' <expression> ')'
     * The atom should already have been consumed, so we can begin by parsing
     * the parenthesized expression.
     */
    const enum TermResultType type = parse_parens(ctx,
        bld, error_text, source_ptr, num_args, arg_names, out_term);
    if(type == eTermImmediate){
        /* If intrinsics can be optimized, then we can apply the operation */
        if(DC_OPTIMIZE_INTRINSIC){
            out_term->immediate = immediate_operation(out_term->immediate);
        }
        else{
            DC_X_BuildPushImmediate(ctx, bld, (float)out_term->immediate);
            operation(ctx, bld);
            return eTermPushed;
        }
    }
    else if(type == eTermArgument){
        DC_X_BuildPushArg(ctx, bld, out_term->argument);
        operation(ctx, bld);
        return eTermPushed;
    }
    else if(type == eTermPushed){
        operation(ctx, bld);
    }
    return type;
}   

/* Parses a term, which can be a value, a parenthesized expression, or a
 * builtin operation */
static enum TermResultType parse_term(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term){
    
    union TermResult result;
    enum TermResultType type;
    /* Check for a parentheszied expression. */
    if(**source_ptr == '('){
        return parse_parens(ctx,
            bld, error_text, source_ptr, num_args, arg_names, out_term);
    }

    /* Builtins are <atom> '(' <expression> ')'
     * Search for the <atom> '(', since then we can re-use the parenthesized
     * expression parsing logic for the argument. */
#define DC_BUILTIN(NAME, IMMEDIATE, PUSH) do{\
        if(strncmp(*source_ptr, ( NAME "(" ), sizeof(NAME))==0){\
            source_ptr[0] += sizeof(NAME) - 1;\
            return builtin(ctx, bld, error_text, source_ptr, num_args,\
                arg_names, out_term, (IMMEDIATE), (PUSH));\
        }\
    }while(0)

    DC_BUILTIN("sin", arithmetic_operation_sin, DC_X_BuildSin);
    DC_BUILTIN("cos", arithmetic_operation_cos, DC_X_BuildCos);
    DC_BUILTIN("sqrt", arithmetic_operation_sqrt, DC_X_BuildSqrt);
    
    /* If it wasn't a builtin or a parenthesized expression, it is a value. */
    type = parse_value(source_ptr, num_args, arg_names, &result);
    switch(type){
        /* Convert any errors to error text. */
        case eTermSyntaxError:
            break;
        case eTermInvalidArgNumber:
            snprintf(error_text,
                0x100,
                "Arg %li is over the maximum of %u",
                result.int_error,
                num_args);
            break;
        case eTermInvalidArgName:
        {
            const char error_msg[] = "Invalid argument name ";
            const unsigned arg_name_size =
                (result.str_error.length + sizeof(error_msg) > 0x100) ?
                (0x100 - sizeof(error_msg)) : result.str_error.length;
            
            memcpy(error_text, error_msg, sizeof(error_msg) - 1);
            memcpy(error_text + sizeof(error_msg) - 1,
                result.str_error.string,
                arg_name_size);
            /* NULL-terminate */
            error_text[sizeof(error_msg) + arg_name_size - 1] = 0;
        }
            break;
        case eTermImmediate:
            if(DC_OPTIMIZE){
                out_term->immediate = result.term.immediate;
            }
            else{
                DC_X_BuildPushImmediate(ctx, bld, (float)result.term.immediate);
                type = eTermPushed;
            }
            break;
        case eTermArgument:
            if(DC_OPTIMIZE){
                out_term->argument = result.term.argument;
            }
            else{
                DC_X_BuildPushArg(ctx, bld, result.term.argument);
                type = eTermPushed;
            }
            break;
        case eTermPushed:
            break;
    }
    return type;
}

static enum TermResultType parse_generic(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term,
    parser_callback parse_callback,
    const struct ParseOperation *operations,
    unsigned num_operations){
    
    union TermType term;
    const char *source;
    enum TermResultType type = parse_callback(
        ctx, bld, error_text, source_ptr, num_args, arg_names, &term);
    
    switch(type){
        case eTermPushed:
            /* FALLTHROUGH */
        case eTermImmediate:
            /* FALLTHROUGH */
        case eTermArgument:
            source = skip_whitespace(*source_ptr);
            while(*source != '\0'){
                unsigned i;
                source = skip_whitespace(source);
                for(i = 0; i < num_operations; i++){
                    if(*source == operations[i].operator_char){
                        union TermType next_term;
                        enum TermResultType next_type;
                        
                        /* Skip past the operator. */
                        source = skip_whitespace(++source);
                        
                        next_type = parse_callback(ctx, bld, error_text, &source,
                            num_args, arg_names, &next_term);
                        
                        if(next_type == eTermImmediate){
                            if(type == eTermImmediate){
                                term.immediate = operations[i].immediate_op(
                                    term.immediate, next_term.immediate);
                            }
                            else{
                                const float imm = (float)next_term.immediate;
                                /* Flush the first argument */
                                if(type == eTermArgument){
                                    /* TODO: We /might/ be able to label
                                     * operators that are transitive and
                                     * re-order the arguments here. */
                                    DC_X_BuildPushArg(ctx, bld, term.argument);
                                    type = eTermPushed;
                                }
                                /* else the arg was pushed. */
                                
                                /* TODO: We could parse the next term and then
                                 * flush? */
                                if(DC_OPTIMIZE_FETCH){
                                    operations[i].build_imm_op(ctx, bld, imm);
                                }
                                else{
                                    DC_X_BuildPushImmediate(ctx, bld, imm);
                                    operations[i].build_op(ctx, bld);
                                }
                            }
                            break;
                        }
                        else if(next_type == eTermArgument){
                            const unsigned short arg = next_term.argument;
                            /* Flush the first argument */
                            if(type == eTermImmediate){
                                const float imm = (float)term.immediate;
                                DC_X_BuildPushImmediate(ctx, bld, imm);
                                type = eTermPushed;
                            }
                            else if(type == eTermArgument){
                                DC_X_BuildPushArg(ctx, bld, term.argument);
                                type = eTermPushed;
                            }
                            
                            if(DC_OPTIMIZE_FETCH){
                                operations[i].build_arg_op(ctx, bld, arg);
                            }
                            else{
                                DC_X_BuildPushArg(ctx, bld, arg);
                                operations[i].build_op(ctx, bld);
                            }
                            break;
                        }
                        else if(next_type == eTermPushed){
                            /* TODO: This seems like it's incorrect? */
                            /* Flush the first argument */
                            if(type == eTermImmediate){
                                const float imm = (float)term.immediate;
                                DC_X_BuildPushImmediate(ctx, bld, imm);
                                type = eTermPushed;
                            }
                            else if(type == eTermArgument){
                                DC_X_BuildPushArg(ctx, bld, term.argument);
                                type = eTermPushed;
                            }
                            operations[i].build_op(ctx, bld);
                            break;
                        }
                        else{
                            /* Handle all errors */
                            return next_type;
                        }
                    }
                }
                
                /* We did not find a matching operation. */
                if(i == num_operations)
                    break;
            }
            
            source_ptr[0] = source;
            if(type == eTermImmediate){
                out_term->immediate = term.immediate;
            }
            else if(type == eTermArgument){
                out_term->immediate = term.argument;
            }
            return type;
        default:
            return type;
    }
    fputs("INTERNAL ERROR\n", stderr);
    abort();
    return 0;
}

/* Implements parsing terms separated by `/' and `*' */
static enum TermResultType parse_mul_ops(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term){
    
    return parse_generic(ctx,
        bld,
        error_text,
        source_ptr,
        num_args,
        arg_names,
        out_term,
        parse_term,
        dc_mul_ops,
        DC_NUM_MUL_OPS);
}

/* Implements parsing terms separated by `+' and `-', calling into
 * parse_mul_ops for each term. */
static enum TermResultType parse_add_ops(struct DC_X_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term){
    
    return parse_generic(ctx,
        bld,
        error_text,
        source_ptr,
        num_args,
        arg_names,
        out_term,
        parse_mul_ops,
        dc_add_ops,
        DC_NUM_ADD_OPS);
}

struct DC_Calculation *DC_Compile(struct DC_Context *dc_ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error){
    
    struct DC_X_Context *const ctx = (struct DC_X_Context *)dc_ctx;
    char error_msg[0x100];
    struct DC_X_CalculationBuilder *const bld =
        DC_X_CreateCalculationBuilder(ctx);
    union TermType term;
    
    source = skip_whitespace(source);
    
    switch(parse_add_ops(ctx,
        bld,
        error_msg,
        &source,
        num_args,
        arg_names,
        &term)){
            case eTermImmediate:
                DC_X_BuildPushImmediate(ctx, bld, (float)term.immediate);
                return (struct DC_Calculation *)DC_X_FinalizeCalculation(ctx,
                    bld);
            case eTermArgument:
                DC_X_BuildPushArg(ctx, bld, term.argument);
                /* FALLTHROUGH */
            case eTermPushed:
                out_error[0] = NULL;
                return (struct DC_Calculation *)DC_X_FinalizeCalculation(ctx,
                    bld);
                /* FALLTHROUGH */
            default:
                {
                    const unsigned error_len =
                        (unsigned)strnlen(error_msg, 0x100);
                    char *const error_txt = malloc(error_len+1);
                    out_error[0] = memcpy(error_txt, error_msg, error_len);
                    error_txt[error_len] = '\0';
                }
                return NULL;
    }
    fputs("INTERNAL ERROR\n", stderr);
    abort();
    return 0;
}

int DC_CompileCalculations(struct DC_Context *dc_ctx,
    int flags,
    unsigned num_calculations,
    const char *const *sources,
    unsigned *num_args,
    const char *const *const *arg_names_array,
    struct DC_Calculation **out_calculations,
    const char **out_error){
    
    struct DC_X_Context *const ctx = (struct DC_X_Context *)dc_ctx;
    char error_msg[0x100];
    unsigned i, first_error = 0;
    
    for(i = 0; i < num_calculations; i++){
        struct DC_X_CalculationBuilder *const bld =
            DC_X_CreateCalculationBuilder(ctx);
        const char *source = skip_whitespace(sources[i]);
        const unsigned nargs = num_args[i];
        const char *const *const args = arg_names_array[i];
        union TermType term;
        const enum TermResultType type =
            parse_add_ops(ctx, bld, error_msg, &source, nargs, args, &term);
        if(type == eTermImmediate){
            DC_X_BuildPushImmediate(ctx, bld, (float)term.immediate);
            out_calculations[i] = (struct DC_Calculation *)
                DC_X_FinalizeCalculation(ctx, bld);
            out_error[i] = NULL;
        }
        else if(type == eTermArgument){
            DC_X_BuildPushArg(ctx, bld, term.argument);
            out_calculations[i] = (struct DC_Calculation *)
                DC_X_FinalizeCalculation(ctx, bld);
            out_error[i] = NULL;
        }
        else if(type == eTermPushed){
            out_calculations[i] = (struct DC_Calculation *)
                DC_X_FinalizeCalculation(ctx, bld);
            out_error[i] = NULL;
        }
        else{
            const unsigned error_len = (unsigned)strnlen(error_msg, 0x100);
            char *const error_txt = malloc(error_len+1);
            out_error[i] = memcpy(error_txt, error_msg, error_len);
            error_txt[error_len] = '\0';
            out_calculations[i] = NULL;
            
            first_error = i+1;
            if((flags & DC_COMPILE_KEEP_GOING) == 0){
                while(++i < num_calculations){
                    out_calculations[i] = NULL;
                    out_error[i] = NULL;
                }
            }
        }
    }
    return first_error;
}


void DC_FreeError(const char *error){
    free((void*)error);
}

void DC_Free(struct DC_Context *ctx, struct DC_Calculation *calc){
    DC_X_Free((struct DC_X_Context *)ctx, (struct DC_X_Calculation*)calc);
}

float DC_Calculate(const struct DC_Calculation *calc, const float *args){
    return DC_X_Calculate((const struct DC_X_Calculation *)calc, args);
}
