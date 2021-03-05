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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGSIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGSIMPL_H
#pragma once

#include "Serialization/IArchive.h"

namespace Serialization {
    inline void BitFlagsWrapper::Serialize(IArchive& ar)
    {
        const Serialization::CEnumDescription& desc = *description;
        int count = desc.count();
        if (ar.IsInput())
        {
            int previousValue = *variable;
            for (int i = 0; i < count; ++i)
            {
                int flagValue = desc.valueByIndex(i);
                if (!(flagValue & visibleMask))
                {
                    continue;
                }
                bool flag = (previousValue & flagValue) == flagValue;
                bool previousFlag = flag;
                ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
                if (flag != previousFlag)
                {
                    if (flag)
                    {
                        *variable |= flagValue;
                    }
                    else
                    {
                        *variable &= ~flagValue;
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < count; ++i)
            {
                int flagValue = desc.valueByIndex(i);
                if (!(flagValue & visibleMask))
                {
                    continue;
                }
                bool flag = (*variable & flagValue) == flagValue;
                ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
            }
        }
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_BITFLAGSIMPL_H
