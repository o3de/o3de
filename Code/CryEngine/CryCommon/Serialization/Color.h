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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLOR_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLOR_H
#pragma once

#include <Serialization/IArchive.h>
#include <Serialization/Decorators/Range.h>

template<typename T>
inline bool Serialize(Serialization::IArchive& ar, Color_tpl<T>& c, const char* name, const char* label);

namespace Serialization
{
    struct Vec3AsColor
    {
        Vec3& v;
        Vec3AsColor(Vec3& v)
            : v(v) {}

        void Serialize(Serialization::IArchive& ar)
        {
            ar(Range(v.x, 0.0f, 1.0f), "r", "^");
            ar(Range(v.y, 0.0f, 1.0f), "g", "^");
            ar(Range(v.z, 0.0f, 1.0f), "b", "^");
        }
    };

    inline bool Serialize(Serialization::IArchive& ar, Vec3AsColor& c, const char* name, const char* label)
    {
        if (ar.IsEdit())
        {
            return ar(Serialization::SStruct(c), name, label);
        }
        else
        {
            typedef float (* Array)[3];
            return ar(*((Array) & c.v.x), name, label);
        }
    }
}

#include "ColorImpl.h"
#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLOR_H
