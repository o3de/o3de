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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGE_H
#pragma once

namespace Serialization
{
    template<class T>
    struct RangeDecorator
    {
        T* value;
        T softMin;
        T softMax;
        T hardMin;
        T hardMax;
    };

    template<class T>
    RangeDecorator<T> Range(T& value, T hardMin, T hardMax)
    {
        RangeDecorator<T> r;
        r.value = &value;
        r.softMin = hardMin;
        r.softMax = hardMax;
        r.hardMin = hardMin;
        r.hardMax = hardMax;
        return r;
    }

    template<class T>
    RangeDecorator<T> Range(T& value, T softMin, T softMax, T hardMin, T hardMax)
    {
        RangeDecorator<T> r;
        r.value = &value;
        r.softMin = softMin;
        r.softMax = softMax;
        r.hardMin = hardMin;
        r.hardMax = hardMax;
        return r;
    }

    namespace Decorators
    {
        // Obsolete name, will be removed. Please use Serialization::Range instead.
        using Serialization::Range;
    }
}

#include "RangeImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_RANGE_H
