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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGS_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGS_H
#pragma once

#include <Serialization/Enum.h>

namespace Serialization {
    class IArchive;

    struct BitFlagsWrapper
    {
        int* variable;
        unsigned int visibleMask;
        const CEnumDescription* description;

        void Serialize(IArchive& ar);
    };

    template<class Enum>
    BitFlagsWrapper BitFlags(Enum& value)
    {
        BitFlagsWrapper wrapper;
        wrapper.variable = (int*)&value;
        wrapper.visibleMask = ~0U;
        wrapper.description = &getEnumDescription<Enum>();
        return wrapper;
    }

    template<class Enum>
    BitFlagsWrapper BitFlags(int& value, int visibleMask = ~0)
    {
        BitFlagsWrapper wrapper;
        wrapper.variable = &value;
        wrapper.visibleMask = visibleMask;
        wrapper.description = &getEnumDescription<Enum>();
        return wrapper;
    }

    template<class Enum>
    BitFlagsWrapper BitFlags(unsigned int& value, unsigned int visibleMask = ~0)
    {
        BitFlagsWrapper wrapper;
        wrapper.variable = (int*)&value;
        wrapper.visibleMask = visibleMask;
        wrapper.description = &getEnumDescription<Enum>();
        return wrapper;
    }
}

#include "BitFlagsImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGS_H
