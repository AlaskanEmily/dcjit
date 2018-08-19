DCJIT
=====

JIT-compiler for simple mathematical expressions 
------------------------------------------------

DCJIT is a very small and simple jit-compiler for floating point mathematical expressions.
It includes constant folding, compound expressions, trigonometic and sqrt functions. There is
also an interpreter implemented in C++. The JIT compiler can evaluate constant expressions and
use them in later subexpressions fully computer, rather than outputting machine code that will
compute a constant value.

DCJIT outputs reasonably well optimized code for x86. Many optimizations are implemented in a
platform-independant way. Code generation is implemented in the native assembly of the platform.

It was created for the Z2 game engine to JIT-compile mathematical expressions related to the
physics engine. It is intended for similar situations, to allow a few equations to be runtime
modifiable without requiring a full scripting language. It provides a much lighterweight JIT
compiler than almost any scripting language.

A (very) simple command-line application to test expression computation using libdcjit is included.

DCJIT is licensed under the MPL 2.0 license.
