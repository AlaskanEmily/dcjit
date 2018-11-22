// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef LIBDCJIT_DC_BYTECODE_HPP
#define LIBDCJIT_DC_BYTECODE_HPP
#pragma once

// Interface for the bytecode which is used for root finding and the interpreter.
// Note that no backends except the interpreter use bytecode.

// Op bytes are two four-bit nibbles.
//
// The least-significant bits are the opcode. The most-significant bits are the 'addressing mode',
// which determines if the operands are constants or
// variables.
//
// Argument pushes are converted to argument indices at compile time, so all
// argument values are index-only.

#include <string.h>
#include <vector>
#include <stack>

namespace DC {
namespace Bytecode {

// The OpType enum is the least significant nibble of each code. The second
// specifies the subtype (see BinaryType and UnaryType).

// Enum for the operation.
enum OpType {
    eArgument,
    eImmediate,
    eUnary,
    eBinary
};

// Binary operation type.
enum BinaryType {
    eAdd,
    eSub,
    eDiv,
    eMul
};

// Unary operation type.
enum UnaryType {
    eSin,
    eCos,
    eSqrt,
    ePop
};

// Bytecode container.
class Bytecode {
public:
    typedef unsigned char byte;

private:
    std::vector<byte> m_bytecode;
    
    template<BinaryType OpType>
    static inline byte EncodeBinary(){
        return static_cast<byte>(eBinary) | (static_cast<byte>(OpType) << 4);
    }
    
    template<UnaryType OpType>
    static inline byte EncodeUnary(){
        return static_cast<byte>(eUnary) | (static_cast<byte>(OpType) << 4);
    }
    
public:
    
    class iterator {
        std::vector<byte>::const_iterator m_iter;
        
    public:
        
        explicit iterator(std::vector<byte>::const_iterator iter)
          : m_iter(iter){}
        
        iterator(const iterator &other)
          : m_iter(other.m_iter){}
        
        inline iterator &operator=(const iterator &other){
            if(&other != this)
                m_iter = other.m_iter;
            return *this;
        }
        
        inline bool operator==(const iterator &other) const {
            return &other == this || m_iter == other.m_iter;
        }
        
        inline bool operator!=(const iterator &other) const {
            return &other != this && m_iter != other.m_iter;
        }
        
        inline OpType opType() const{
            return static_cast<OpType>((*m_iter) & 0x0F);
        }
        
        BinaryType readBinaryOp();
        
        UnaryType readUnaryOp();
        
        float readImmediate();
        
        unsigned short readArgument();
    };
    
    void writeImmediate(float imm);
    
    void writeArgument(unsigned short arg);
    
    template<BinaryType OpType>
    inline void writeBinary(){
        m_bytecode.push_back(EncodeBinary<OpType>());
    }
    
    template<BinaryType OpType>
    inline void writeBinaryImmediate(float imm){
        writeImmediate(imm);
        writeBinary<OpType>();
    }
    
    template<BinaryType OpType>
    inline void writeBinaryArgument(unsigned short arg){
        writeArgument(arg);
        writeBinary<OpType>();
    }
    
    template<UnaryType OpType>
    inline void writeUnary(){
        m_bytecode.push_back(EncodeUnary<OpType>());
    }
    
    template<UnaryType OpType>
    inline void writeUnaryImmediate(float imm){
        writeImmediate(imm);
        writeUnary<OpType>();
    }
    
    template<UnaryType OpType>
    inline void writeUnaryArgument(unsigned short arg){
        writeArgument(arg);
        writeUnary<OpType>();
    }
    
    iterator begin() const;
    inline iterator cbegin() const { return begin(); }

    iterator end() const;
    inline iterator cend() const { return end(); }
};

} // namespace Bytecode
} // namespace DC

#endif /* LIBDCJIT_DC_BYTECODE_HPP */
