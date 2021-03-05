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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SLIDER_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SLIDER_H
#pragma once

namespace Serialization
{
    class IArchive;

    struct SSliderF
    {
        SSliderF(float* value, float _minLimit, float _maxLimit)
            : valuePointer(value)
            , minLimit(_minLimit)
            , maxLimit(_maxLimit)
        {
        }

        SSliderF()
            : valuePointer(0)
            , minLimit(0.0f)
            , maxLimit(1.0f)
        {
        }


        float* valuePointer;
        float minLimit;
        float maxLimit;
    };

    struct SSliderI
    {
        SSliderI(int* value, int minLimit, int maxLimit)
            : valuePointer(value)
            , minLimit(minLimit)
            , maxLimit(maxLimit)
        {
        }

        SSliderI()
            : valuePointer(0)
            , minLimit(0)
            , maxLimit(1)
        {
        }

        int* valuePointer;
        int minLimit;
        int maxLimit;
    };


    inline SSliderF Slider(float& value, float minLimit, float maxLimit)
    {
        return SSliderF(&value, minLimit, maxLimit);
    }

    inline SSliderI Slider(int& value, int minLimit, int maxLimit)
    {
        return SSliderI(&value, minLimit, maxLimit);
    }

    bool Serialize(IArchive& ar, SSliderF& slider, const char* name, const char* label);
    bool Serialize(IArchive& ar, SSliderI& slider, const char* name, const char* label);

    namespace Decorators
    {
        // OBSOLETE NAME, please use Serialization::Slider instead (without Decorators namespace)
        using Serialization::Slider;
    }
}

#include "SliderImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_SLIDER_H
