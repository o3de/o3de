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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGEIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGEIMPL_H
#pragma once

namespace Serialization
{
    template<class T>
    bool Serialize(IArchive& ar, RangeDecorator<T>& value, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            if (!ar(SStruct::ForEdit(value), name, label))
            {
                return false;
            }
        }
        else if (!ar(*value.value, name, label))
        {
            return false;
        }

        if (ar.IsInput())
        {
            if (*value.value < value.hardMin)
            {
                *value.value = value.hardMin;
            }
            if (*value.value > value.hardMax)
            {
                *value.value = value.hardMax;
            }
        }
        return true;
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGEIMPL_H
