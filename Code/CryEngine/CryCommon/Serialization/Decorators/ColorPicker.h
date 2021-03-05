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
#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_COLORPICKER_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_COLORPICKER_H
#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>

#include <ISystem.h>

namespace Serialization
{
    class IArchive;

    struct ColorPicker
    {
        ColorF* color;

        explicit ColorPicker(ColorF& color_)
            : color(&color_)
        {
        }

        // the function should stay virtual to ensure cross-dll calls are using right heap
        virtual void SetColor(const ColorF* color_){* color = *color_; }
    };

    bool Serialize(Serialization::IArchive& ar, Serialization::ColorPicker& value, const char* name, const char* label);
} // namespace Serialization

#include "ColorPickerImpl.h"

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_DECORATORS_COLORPICKER_H
