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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLORIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLORIMPL_H
#pragma once

#include "Color.h"

//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableColor_tpl
    : Color_tpl<T>
{
    static float ColorRangeMin(float) { return 0.0f; }
    static float ColorRangeMax(float) { return 1.0f; }
    static unsigned char ColorRangeMin(unsigned char) { return 0; }
    static unsigned char ColorRangeMax(unsigned char) { return 255; }

    void Serialize(Serialization::IArchive& ar)
    {
        ar(Serialization::Range(Color_tpl<T>::r, ColorRangeMin(Color_tpl<T>::r), ColorRangeMax(Color_tpl<T>::r)), "r", "^");
        ar(Serialization::Range(Color_tpl<T>::g, ColorRangeMin(Color_tpl<T>::g), ColorRangeMax(Color_tpl<T>::g)), "g", "^");
        ar(Serialization::Range(Color_tpl<T>::b, ColorRangeMin(Color_tpl<T>::b), ColorRangeMax(Color_tpl<T>::b)), "b", "^");
        ar(Serialization::Range(Color_tpl<T>::a, ColorRangeMin(Color_tpl<T>::a), ColorRangeMax(Color_tpl<T>::a)), "a", "^");
    }
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Color_tpl<T>& c, const char* name, const char* label)
{
    if (ar.IsEdit())
    {
        return Serialize(ar, static_cast<SerializableColor_tpl<T>&>(c), name, label);
    }
    else
    {
        typedef T (& Array)[4];
        return ar((Array)c, name, label);
    }
}

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_COLORIMPL_H
