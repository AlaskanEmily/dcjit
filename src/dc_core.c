/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dc.h"
#include "dc_backend.h"

/* needed for strncpy on some systems */
#define _BSD_SOURCE

/* Used for snprintf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef DC_OPTIMIZE
#define DC_OPTIMIZE 0
#endif

#ifndef DC_OPTIMIZE_INTRINSIC
#define DC_OPTIMIZE_INTRINSIC DC_OPTIMIZE
#endif


#ifdef _MSC_VER
    #define DC_STRNCPY(DEST, LEN, TXT) strncpy_s((DEST), (LEN),  (TXT), _TRUNCATE)
#else
    #define DC_STRNCPY(DEST, LEN, TXT) do {\
        char *const DC_STRNCPY_dst = (DST);\
        const long DC_STRNCPY_len = (LEN);\
        strncpy(DC_STRNCPY_dst, (TXT), DC_STRNCPY_len);\
        DC_STRNCPY_dst[DC_STRNCPY_len-1] = '\0';\
    }while(0)
#endif

/* General parsing components. */
enum TermResultType {
    eTermImmediate,
    eTermArgument,
    eTermPushed,
    eTermSyntaxError,
    eTermInvalidArgNumber,
    eTermInvalidArgName
};

union TermType {
    double immediate;
    unsigned short argument;
};

typedef double(*arithmetic_operation)(double, double);
typedef double(*unary_operation)(double);
typedef enum TermResultType(*parser_callback)(struct DC_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term);

static double arithmetic_operation_add(double a, double b) { return a + b; }
static double arithmetic_operation_sub(double a, double b) { return a - b; }
static double arithmetic_operation_mul(double a, double b) { return a * b; }
static double arithmetic_operation_div(double a, double b) { return a / b; }
static double arithmetic_operation_sin(double a){ return sin(a); }
static double arithmetic_operation_cos(double a){ return cos(a); }
static double arithmetic_operation_sqrt(double a){ return sqrt(a); }
typedef void (*build_push_operation)(struct DC_X_Context*, struct DC_X_CalculationBuilder*);

struct DC_Context{
    struct DC_X_Context *ctx;
};

struct DC_Calculation{
    struct DC_X_Calculation *calc;
    char error[0x100];
};

struct DC_Context *DC_CreateContext(void){
    struct DC_Context *const ctx = malloc(sizeof(struct DC_Context *));
    ctx->ctx = DC_X_CreateContext();
    return ctx;
}

void DC_FreeContext(struct DC_Context *ctx){
    DC_X_FreeContext(ctx->ctx);
    free(ctx);
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

/* Parses an integer. */
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

/* Parses a double. */
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
static enum TermResultType parse_add_ops(struct DC_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term);

/* Parses a value. This can be a literal, or an argument name or number. */
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

static enum TermResultType parse_parens(struct DC_Context *ctx,
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
static enum TermResultType builtin(struct DC_Context *ctx,
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
            DC_X_BuildPushImmediate(ctx->ctx, bld, (float)out_term->immediate);
            operation(ctx->ctx, bld);
            return eTermPushed;
        }
    }
    else if(type == eTermArgument){
        DC_X_BuildPushArg(ctx->ctx, bld, out_term->argument);
        operation(ctx->ctx, bld);
        return eTermPushed;
    }
    else if(type == eTermPushed){
        operation(ctx->ctx, bld);
    }
    return type;
}   

/* Parses a term, which can be a value, a parenthesized expression, or a
 * builtin operation */
static enum TermResultType parse_term(struct DC_Context *ctx,
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
                DC_X_BuildPushImmediate(ctx->ctx, bld, (float)result.term.immediate);
                type = eTermPushed;
            }
            break;
        case eTermArgument:
            if(DC_OPTIMIZE){
                out_term->argument = result.term.argument;
            }
            else{
                DC_X_BuildPushArg(ctx->ctx, bld, result.term.argument);
                type = eTermPushed;
            }
            break;
        case eTermPushed:
            break;
    }
    return type;
}

static enum TermResultType parse_generic(struct DC_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    union TermType *out_term,
    parser_callback parse_callback,
    const char first_char,
    const char second_char,
    arithmetic_operation first_op,
    arithmetic_operation second_op,
    build_push_operation first_build,
    build_push_operation second_build){
    
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
                source = skip_whitespace(source);
                if(*source == first_char || *source == second_char){
                    const int is_first = (*source == first_char);
                    
                    union TermType next_term;
                    enum TermResultType next_type;
                    
                    /* Skip past the operator. */
                    source = skip_whitespace(++source);
                    
                    next_type = parse_callback(ctx, bld, error_text, &source,
                        num_args, arg_names, &next_term);
                    
                    switch(next_type){
                        case eTermImmediate:
                            if(type == eTermImmediate){
                                term.immediate = (is_first) ?
                                    first_op(term.immediate,
                                        next_term.immediate) :
                                    second_op(term.immediate,
                                        next_term.immediate);
                                continue;
                            }
                            
                            /* Flush the first argument */
                            if(type == eTermArgument){
                                DC_X_BuildPushArg(ctx->ctx,
                                    bld, term.argument);
                                type = eTermPushed;
                            }
                            
                            /* TODO: We could parse the next term and then flush */
                            DC_X_BuildPushImmediate(ctx->ctx,
                                bld, (float)next_term.immediate);
                            if(is_first)
                                first_build(ctx->ctx, bld);
                            else
                                second_build(ctx->ctx, bld);
                            continue;
                        case eTermArgument:
                            
                            /* Flush the first argument */
                            if(type == eTermImmediate){
                                DC_X_BuildPushImmediate(ctx->ctx,
                                    bld, (float)term.immediate);
                                type = eTermPushed;
                            }
                            else if(type == eTermArgument){
                                DC_X_BuildPushArg(ctx->ctx,
                                    bld, term.argument);
                                type = eTermPushed;
                            }
                            DC_X_BuildPushArg(ctx->ctx,
                                bld, next_term.argument);
                                
                            /* FALLTHROUGH */
                        case eTermPushed:
                            if(is_first)
                                first_build(ctx->ctx, bld);
                            else
                                second_build(ctx->ctx, bld);
                            continue;
                            
                        /* Handle all errors */
                        default:
                            return next_type;
                    }
                }
                else{
                    break;
                }
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
static enum TermResultType parse_mul_ops(struct DC_Context *ctx,
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
        '*',
        '/',
        arithmetic_operation_mul,
        arithmetic_operation_div,
        DC_X_BuildMul,
        DC_X_BuildDiv);
}

/* Implements parsing terms separated by `+' and `-', calling into
 * parse_mul_ops for each term. */
static enum TermResultType parse_add_ops(struct DC_Context *ctx,
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
        '+',
        '-',
        arithmetic_operation_add,
        arithmetic_operation_sub,
        DC_X_BuildAdd,
        DC_X_BuildSub);
}

struct DC_Calculation *DC_Compile(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names){
    
    struct DC_Calculation *const calc = malloc(sizeof(struct DC_Calculation));
    struct DC_X_CalculationBuilder *const bld =
        DC_X_CreateCalculationBuilder(ctx->ctx);
    
    union TermType term;
    calc->calc = NULL;
    source = skip_whitespace(source);
    
    switch(parse_add_ops(ctx,
        bld,
        calc->error,
        &source,
        num_args,
        arg_names,
        &term)){
            case eTermImmediate:
                DC_X_BuildPushImmediate(ctx->ctx, bld, (float)term.immediate);
                calc->error[0] = 0;
                calc->calc = DC_X_FinalizeCalculation(ctx->ctx, bld);
                return calc;
            case eTermArgument:
                DC_X_BuildPushArg(ctx->ctx, bld, term.argument);
                /* FALLTHROUGH */
            case eTermPushed:
                calc->error[0] = 0;
                calc->calc = DC_X_FinalizeCalculation(ctx->ctx, bld);
                /* FALLTHROUGH */
            default:
                return calc;
    }
    fputs("INTERNAL ERROR ", stderr);
    fputs(__FUNCTION__, stderr);
    fputc('\n', stderr);
    abort();
    return 0;
}

void DC_Free(struct DC_Context *ctx, struct DC_Calculation *calc){
    if(calc->calc != NULL)
        DC_X_Free(ctx->ctx, calc->calc);
}

const char *DC_GetError(const struct DC_Calculation *calc){
    if(calc->error[0])
        return calc->error;
    else
        return NULL;
}

float DC_Calculate(const struct DC_Calculation *calc, const float *args){
    return DC_X_Calculate(calc->calc, args);
}
