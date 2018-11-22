// Copyright (c) 2018, Transnat Games
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "dc_bytecode.hpp"

typedef DC::Bytecode::Bytecode::byte byte;

namespace DC {

template<typename T>
static inline T read(std::vector<byte>::const_iterator &iter) {
    T t;
    byte *const bytes = reinterpret_cast<byte*>(&t);
    for(size_t i = 0; i < sizeof(T); i++)
        bytes[i] = *iter++;
    return t;
}

template<typename T>
static inline void write(std::vector<byte> &vector, T that){
    const size_t bytecode_size = vector.size();
    vector.resize(bytecode_size + sizeof(T));
    memcpy(&(vector[bytecode_size]), &that, sizeof(T));
}

template<>
static inline void write<byte>(std::vector<byte> &vector, byte that){
    vector.push_back(that);
}

namespace Bytecode {

void Bytecode::writeImmediate(float imm){
    write<byte>(m_bytecode, static_cast<byte>(eImmediate));
    write<float>(m_bytecode, imm);
}

void Bytecode::writeArgument(unsigned short arg){
    write<byte>(m_bytecode, static_cast<byte>(eArgument));
    write<unsigned short>(m_bytecode, arg);
}

BinaryType Bytecode::iterator::readBinaryOp(){
    return static_cast<BinaryType>((*m_iter++) >> 4);
}

UnaryType Bytecode::iterator::readUnaryOp(){
    return static_cast<UnaryType>((*m_iter++) >> 4);
}

float Bytecode::iterator::readImmediate(){
    m_iter++;
    return read<float>(m_iter);
}

unsigned short Bytecode::iterator::readArgument(){
    m_iter++;
    return read<unsigned short>(m_iter);
}

Bytecode::iterator Bytecode::begin() const{
    return iterator(m_bytecode.begin());
}

Bytecode::iterator Bytecode::end() const{
    return iterator(m_bytecode.end());
}

} // namespace Bytecode
} // namespace DC
