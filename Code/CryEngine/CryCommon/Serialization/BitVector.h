// Copyright (c) 2012 Crytek GmbH
// Authors: Evgeny Andreeshchev, Alexander Kotliar
// Based on: Yasli - the serialization library.
// Modifications copyright Amazon.com, Inc. or its affiliates

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTOR_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTOR_H
#pragma once


namespace Serialization{

class IArchive;
template<class Enum>
class BitVector
{
public:
    BitVector(int value = 0) : value_(value) {}

    operator int&() { return value_; }
    operator int() const { return value_; }

    BitVector& operator|= (Enum value) { value_ |= value; return *this; }
    BitVector& operator|= (int value) { value_ |= value; return *this; }
    BitVector& operator&= (int value) { value_ &= value; return *this; }

    void Serialize(IArchive& ar);
private:
    int value_;
};

template<class Enum>
bool Serialize(Serialization::IArchive& ar, Serialization::BitVector<Enum>& value, const char* name, const char* label);

}

#include "BitVectorImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTOR_H
