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

// Description : 'float' explicit specialization of the class template
//               'TAnimSplineTrack'
// Notice      : Should be included in AnimSplineTrack.h only


#ifndef CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_FLOATSPECIALIZATION_H
#define CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_FLOATSPECIALIZATION_H
#pragma once


template <>
inline TAnimSplineTrack<float>::TAnimSplineTrack()
{
    AllocSpline();
    m_flags = 0;
    m_defaultValue = 0;
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
    m_node = nullptr;
    m_trackMultiplier = 1.0f;
}
template <>
inline void TAnimSplineTrack<float>::GetValue(float time, float& value, bool applyMultiplier)
{
    if (GetNumKeys() == 0)
    {
        value = m_defaultValue;
    }
    else
    {
        m_spline->interpolate(time, value);
    }

    if (applyMultiplier && m_trackMultiplier != 1.0f)
    {
        value /= m_trackMultiplier;
    }
}
template <>
inline EAnimCurveType TAnimSplineTrack<float>::GetCurveType() { return eAnimCurveType_TCBFloat; }
template <>
inline EAnimValue TAnimSplineTrack<float>::GetValueType() { return eAnimValue_Float; }
template <>
inline void TAnimSplineTrack<float>::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
{
    
    if (!bDefault)
    {
        ITcbKey key;
        if (applyMultiplier && m_trackMultiplier != 1.0f)
        {
            key.SetValue(value * m_trackMultiplier);
        }
        else
        {
            key.SetValue(value);
        }

        SetKeyAtTime(time, &key);
    }
    else
    {
        if (applyMultiplier && m_trackMultiplier != 1.0f)
        {
            m_defaultValue = value * m_trackMultiplier;
        }
        else
        {
            m_defaultValue = value;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimSplineTrack<float>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;
    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    sprintf_s(str, "%.2f", k.value);
}

#endif // CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_FLOATSPECIALIZATION_H
