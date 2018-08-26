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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Context to create and run calculations.
 */
struct DC_Context;

/**
 * @brief A calculation that has been compiled and can be run.
 */
struct DC_Calculation;

/**
 * @brief A calculation that has been compiled, but not made executable yet.
 *
 * @sa DC_CompileCalculation
 */
struct DC_CalculationBuilder;

/**
 * @brief Creates a context.
 */
struct DC_Context *DC_CreateContext(void);

/**
 * @brief Frees a context.
 *
 * All calculations created in this context must be freed with DC_Free
 * before the context is freed.
 */
void DC_FreeContext(struct DC_Context *ctx);

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
struct DC_Calculation *DC_Compile(struct DC_Context *ctx,
    const char *source,
    unsigned num_args,
    const char *const *arg_names,
    const char **out_error);

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
int DC_CompileCalculations(struct DC_Context *ctx,
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
void DC_FreeError(const char *error);

/**
 * @brief Frees a calculation
 */
void DC_Free(struct DC_Context *ctx, struct DC_Calculation *);

/**
 * @brief Runs a calculation.
 */
float DC_Calculate(const struct DC_Calculation *, const float *args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDCJIT_DC_H */
