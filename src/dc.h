/* Copyright (c) 2018, Transnat Games
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef LIBDCJIT_DC_H
#define LIBDCJIT_DC_H

/*
 * The public API for the libdcjit library.
 */
#if defined _WIN32 && !defined __CYGWIN__
    #define DC_API_CALL __stdcall
    #ifdef DCJIT_INTERNAL
        #define DC_API __declspec(dllexport) DC_API_CALL
    #else
        #define DC_API __declspec(dllimport) DC_API_CALL
    #endif
#elif defined __CYGWIN__
    #define DC_API_CALL __attribute__((stdcall))
    #define DC_API DC_API_CALL
#elif defined __EMSCRIPTEN__
    #define DC_API_CALL
    #define DC_API __attribute__((used)) DC_API_CALL
#else
    #define DC_API_CALL
    #define DC_API DC_API_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Context to create and run calculations.
 */
struct DC_Context;
typedef struct DC_Context *DC_ContextPtr;

/**
 * @brief A calculation that has been compiled and can be run.
 */
struct DC_Calculation;
typedef struct DC_Calculation *DC_CalculationPtr;

/**
 * @brief Optional bytecode representation of a calculation.
 *
 * This is used for the root-finding API (when enabled).
 *
 * The interpreter backend also makes use of this, but using the bytecode for
 * this explicitly is not exposed in the public API.
 */
struct DC_Bytecode;
typedef struct DC_Bytecode *DC_BytecodePtr;

void DC_API DC_FreeBytecode(struct DC_Bytecode *bc);

/**
 * @brief Creates a context.
 */
DC_ContextPtr DC_API DC_CreateContext(void);

/**
 * @brief Frees a context.
 *
 * All calculations created in this context must be freed with DC_Free
 * before the context is freed.
 */
void DC_API DC_FreeContext(struct DC_Context *ctx);

#define DC_COMPILE_KEEP_GOING 1

/**
 * @brief Compiles a calculation.
 *
 * This function is provided mostly for convenience. Most applications should
 * use DC_CompileCalculations.
 *
 * The language that DCJIT implements is defined as:
 *
 * <expression> ::= <factor> [<mulop> <factor>]
 * <mulop>      ::= '*' | '/'
 * <factor>     ::= <term> [<addop> <term>]
 * <addop>      ::= '+' | '-'
 * <term>       ::= <builtin> | '(' <expression> ')' | <number> | <argument>
 * <builtin>    ::= <func> '(' <expression> ')'
 * <func>       ::= 'sin' | 'cos' | 'sqrt'
 * <number>     ::= '.' {0-9}+ | {0-9}+ ['.' {0-9}*]
 * <argument>   ::= '$'{0-9}+ | {a-zA-Z_}
 *
 * The compiler will compute any constant expressions. For instance, the
 * expression "97.1 * sin(11 + 0.9)" would be fully calculated at compile time
 * and the calculation would just return the pre-computed result. This can be
 * suppressed by defining DC_OPTIMIZE to 0, which is useful for debugging.
 *
 * Usually, it's best to use DC_CompileCalculations and batch all compilation
 * into a single call. This will significantly reduce the amount of memory used
 * since otherwise each calculation will use an entire page (which is 4 KB on
 * x86, and either 4KB or 4 MB on amd64, for instance) where otherwise the
 * calculations can be grouped into a few pages.
 *
 * @param ctx The context to compile the calcuation in.
 * @param source Source code for the calculation
 * @param num_args Number of arguments to the calculation
 * @param arg_names Aliases for the arguments to the calculation
 * @param out_error Receives an error if the compilation fails
 * @return new calculation, or NULL if an error has occured
 */
DC_CalculationPtr DC_API DC_CompileCalculation(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error);

/**
 * @brief Generate bytecide for a calculation.
 *
 * @note Bytecode is only used for the root-finding API.
 *
 * Bytecode is not used for running any calculations.
 *
 * @param ctx The context to compile the calcuation in.
 * @param source Source code for the calculation
 * @param num_args Number of arguments to the calculation
 * @param arg_names Aliases for the arguments to the calculation
 * @param out_error Receives an error if the compilation fails
 * @return Bytecode, or NULL if an error has occured
 *
 * @sa DC_Compile
 */
DC_BytecodePtr DC_API DC_CompileBytecode(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error);

/**
 * @brief Compile a calculation to machine code and/or bytecode.
 *
 * This is equivalent to DC_CompileCalculation but also produces bytecode.
 *
 * @param ctx The context to compile the calcuation in.
 * @param source Source code for the calculation
 * @param num_args Number of arguments to the calculation
 * @param arg_names Aliases for the arguments to the calculation
 * @param out_error Receives an error if the compilation fails
 * @param out_optional_calculation Calculation, or NULL if an error has occured
 * @param out_optional_bytecode Bytecode, or NULL if an error has occured
 *
 * @sa DC_CompileCalculation
 * @sa DC_CompileBytecode
 */
void DC_API DC_Compile(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error,
    DC_CalculationPtr *out_optional_calculation,
    DC_BytecodePtr *out_optional_bytecode);

/**
 * @brief Compiles a set of calculations.
 *
 * This is more efficient than DC_Compile for compiling multiple calculations,
 * as each otherwise each calculation will use an entire page (which is 4 KB on
 * x86, and either 4KB or 4 MB on amd64, for instance) where otherwise the
 * calculations can be grouped into a few pages.
 *
 * Ordinarily, this function will halt immediately if a calculation cannot be
 * compiled. If @p flags includes DC_COMPILE_KEEP_GOING then compilation will
 * continue.
 *
 * @note There is no method to batch-generate bytecode. This is because there
 *   is no significant benefit over calling DC_CompileCalculations for all the
 *   calculations, and then calling DC_CompileBytecode in a loop.
 *
 * @param ctx The context to compile the calcuations in.
 * @param flags bitwise-or'ed flags for compilation.
 * @param num_calculations Number of calculations to compile. The arrays in
 *   @p sources, @p num_args, @p arg_names, @p out_calculations, and
 *   @p out_error must be at least this number of elements.
 * @param sources Array of source code strings for the calculations.
 * @param num_args Array of number of arguments to the calculations
 * @param arg_names Array of argument alias arrays for the calculations
 * @param out_calculations new calculations, or NULL if an error has occured
 * @param out_error Array of errors for the calculations.
 * @return The index of the calculation that returned an error plus one, or
 *   zero if the calculations are completed successfully. If @p flags included
 *   DC_COMPILE_KEEP_GOING, then this is indicates the first calculation that
 *   had an error.
 */
int DC_API DC_CompileCalculations(struct DC_Context *ctx,
    int flags,
    unsigned num_calculations,
    const char *const *sources,
    unsigned *num_args,
    const char *const *const *arg_names,
    struct DC_Calculation **out_calculations,
    const char **out_error);

/**
 * @brief Frees the out_error from DC_Compile
 */
void DC_API DC_FreeError(const char *error);

/**
 * @brief Frees a calculation
 */
void DC_API DC_Free(struct DC_Context *ctx, struct DC_Calculation *);

/**
 * @brief Runs a calculation.
 */
float DC_API DC_Calculate(const struct DC_Calculation *, const float *args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_H */
