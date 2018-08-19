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

#ifndef DC_OPTIMIZE
#define DC_OPTIMIZE 1
#endif

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

/* Parses a float. */
static float parse_float(const char **const source_ptr){
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
                return (float)(negate ? (-value) : value);
            }
        }
        else{
            const float value = (float)numerator;
            source_ptr[0] = source;
            return negate ? (-value) : value;
        }
    }
}

union TermType {
    float immediate;
    unsigned short argument;
};

enum TermResultType {
    eTermImmediate,
    eTermArgument,
    eTermInvalidArgNumber,
    eTermInvalidArgName
};

#define TERM_IS_ERROR(X) ((unsigned)(X) > 1u) /* This is evil. Such is life. */

/* Data for terms. */
union TermResult {
    union TermType term;
    struct {
        const char *string;
        unsigned length;
    } str_error;
    long int_error;
};

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
            return DC_TERM_SUCCESS_IMM(parse_float(&source));
        default:
            /* Parse an arg name */
            {
                const char *const arg_name_start = source;
                unsigned arg_num = 0, arg_name_size = 0;
            next_char:
                {
                    const int c = source[arg_name_size++];
                    if(c == '_' ||
                        (c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c & 0x80)){
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

enum AddOp {
    eAdd,
    eSub
};

enum MulOp {
    eMul,
    eDiv
};


static enum TermResultType parse_term(struct DC_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    float *out_immediate){
    
    union TermResult result;
    switch(parse_value(source_ptr, num_args, arg_names, &result)){
        case eTermInvalidArgNumber:
            snprintf(error_text,
                0x100,
                "Arg %li is over the maximum of %u",
                result.int_error,
                num_args);
            return eTermInvalidArgNumber;
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
            return eTermInvalidArgName;
        case eTermImmediate:
            out_immediate[0] = result.term.immediate;
            return eTermImmediate;
        case eTermArgument:
            DC_X_BuildPushArg(ctx->ctx, bld, result.term.argument);
            return eTermArgument;
    }
    fputs("INTERNAL ERROR", stderr);
    abort();
}

static enum TermResultType parse_mul_ops(struct DC_Context *ctx,
    struct DC_X_CalculationBuilder *bld,
    char error_text[0x100],
    const char **source_ptr,
    unsigned num_args,
    const char *const *arg_names,
    float *out_immediate){
    
    int is_immediate = 0;
    float immediate;
    const char *source;
    
    switch(parse_term(ctx,
        bld,
        error_text,
        source_ptr,
        num_args,
        arg_names,
        &immediate)){
        
        case eTermInvalidArgNumber:
            return eTermInvalidArgNumber;
        case eTermInvalidArgName:
            return eTermInvalidArgName;
        case eTermImmediate:
            /* This will prevent the initial is_immediate set, which stops all
             * future constant folding in this expression. */
            if(!DC_OPTIMIZE){
                DC_X_BuildPushImmediate(ctx->ctx, bld, immediate);
            }
            else{
                is_immediate = 1;
            }
            /* FALLTHROUGH */
        case eTermArgument:
            float next_immediate;
            source = *source_ptr;
            while(*source != '\0'){
                int is_mul = 0;
                source = skip_whitespace(source);
                switch(*source){
                    case '/':
                        is_mul = 0;
                    case '*':
                        source = skip_whitespace(++source);
                        switch(parse_term(ctx,
                            bld,
                            error_text,
                            &source,
                            num_args,
                            arg_names,
                            &next_immediate)){
                            case eTermInvalidArgNumber: /* FALLTHROUGH */
                                return eTermInvalidArgNumber;
                            case eTermInvalidArgName:
                                return eTermInvalidArgName;
                            case eTermImmediate:
                                if(is_immediate){
                                    if(is_mul)
                                        immediate *= next_immediate;
                                    else
                                        immediate /= next_immediate;
                                    break;
                                }
                                /* TODO: We could parse the next term and then flush */
                                DC_X_BuildPushImmediate(ctx->ctx, bld, next_immediate);
                                /* FALLTHROUGH */
                            case eTermArgument:
                                is_immediate = 0;
                                if(is_mul)
                                    DC_X_BuildMul(ctx->ctx, bld);
                                else
                                    DC_X_BuildDiv(ctx->ctx, bld);
                            break;
                        }
                        break;
                    default:
                        break;
                }
            }
    }
    
    if(is_immediate){
        out_immediate[0] = immediate;
        return eTermImmediate;
    }
    else{
        return eTermArgument;
    }
}


struct DC_Calculation *DC_Compile(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names){
    
    struct DC_Calculation *const calc = malloc(sizeof(struct DC_Calculation));
    struct DC_X_CalculationBuilder *const bld =
        DC_X_CreateCalculationBuilder(ctx->ctx);
    
    float immediate;
    
    calc->calc = NULL;
    switch(parse_mul_ops(ctx,
        bld,
        calc->error,
        &source,
        num_args,
        arg_names,
        &immediate)){
        case eTermImmediate:
            DC_X_BuildPushImmediate(ctx->ctx, bld, immediate);
            /* FALLTHROUGH */
        case eTermArgument:
            calc->error[0] = 0; /* FALLTHROUGH */
            calc->calc = DC_X_FinalizeCalculation(ctx->ctx, bld);
        case eTermInvalidArgNumber: /* FALLTHROUGH */
        case eTermInvalidArgName:
            return calc;
    }
    
    fputs("INTERNAL ERROR", stderr);
    abort();
}

void DC_Free(struct DC_Context *ctx, struct DC_Calculation *calc){
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