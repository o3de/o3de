/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTORIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTORIMPL_H
#pragma once

#include "Serialization/BitVector.h"
#include "Serialization/IArchive.h"
#include "Serialization/Enum.h"

namespace Serialization {
    struct BitVectorWrapper
    {
        int* valuePointer;
        int value;
        const CEnumDescription* description;

        explicit BitVectorWrapper(int* _value = 0, const CEnumDescription* _description = 0)
            : valuePointer(_value)
            , description(_description)
        {
            if (valuePointer)
            {
                value = *valuePointer;
            }
        }
        BitVectorWrapper(const BitVectorWrapper& _rhs)
            : value(_rhs.value)
            , description(0)
            , valuePointer(0)
        {
        }

        ~BitVectorWrapper()
        {
            if (valuePointer)
            {
                * valuePointer = value;
            }
        }
        BitVectorWrapper& operator=(const BitVectorWrapper& rhs)
        {
            value = rhs.value;
            return *this;
        }


        void Serialize(IArchive& ar)
        {
            ar(value, "value", "Value");
        }
    };

    template<class Enum>
    void BitVector<Enum>::Serialize(IArchive& ar)
    {
        ar(value_, "value", "Value");
    }
}

template<class Enum>
bool Serialize(Serialization::IArchive& ar, Serialization::BitVector<Enum>& value, const char* name, const char* label)
{
    using namespace Serialization;
    CEnumDescription& desc = getEnumDescription<Enum>();
    if (ar.IsEdit())
    {
        return ar(BitVectorWrapper(&static_cast<int&>(value), &desc), name, label);
    }
    else
    {
        return desc.serializeBitVector(ar, static_cast<int&>(value), name, label);
    }
}


#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_BITVECTORIMPL_H
