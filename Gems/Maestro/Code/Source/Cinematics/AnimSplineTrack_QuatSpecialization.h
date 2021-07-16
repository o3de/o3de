/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


//  Description : 'Quat' explicit specialization of the class template
//                'TAnimSplineTrack'
//  Notice      : Should be included in AnimSplineTrack.h only

#ifndef CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_QUATSPECIALIZATION_H
#define CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_QUATSPECIALIZATION_H
#pragma once


template <>
inline TAnimSplineTrack<Quat>::TAnimSplineTrack()
{
    AllocSpline();
    //m_spline = (Spline*)new spline::TCBQuatSplineInterpolator;
    m_flags = 0;
    m_defaultValue.SetIdentity();
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
    m_node = nullptr;
}

template <>
inline void TAnimSplineTrack<Quat>::GetValue(float time, Quat& value)
{
    if (GetNumKeys() == 0)
    {
        value = m_defaultValue;
    }
    else
    {
        m_spline->interpolate(time, value);
    }
}
template <>
inline EAnimCurveType TAnimSplineTrack<Quat>::GetCurveType() { return eAnimCurveType_TCBQuat; }
template <>
inline EAnimValue TAnimSplineTrack<Quat>::GetValueType() { return eAnimValue_Quat; }
template <>
inline void TAnimSplineTrack<Quat>::SetValue(float time, const Quat& value, bool bDefault)
{
    if (!bDefault)
    {
        ITcbKey key;
        key.SetValue(value);
        SetKeyAtTime(time, &key);
    }
    else
    {
        m_defaultValue = value;
    }
}

//////////////////////////////////////////////////////////////////////////
template <>
inline void TAnimSplineTrack<Quat>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;

    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    Ang3 Angles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(k.value)));
    sprintf_s(str, "%.2f  %.2f  %.2f", Angles.x, Angles.y, Angles.z);
}

#endif // CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_QUATSPECIALIZATION_H
