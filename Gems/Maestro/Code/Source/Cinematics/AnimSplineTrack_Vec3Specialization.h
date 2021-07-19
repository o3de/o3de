/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : 'Vec3' explicit specialization of the class template
//               'TAnimSplineTrack'

#ifndef CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC3SPECIALIZATION_H
#define CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC3SPECIALIZATION_H
#pragma once


template <>
inline TAnimSplineTrack<Vec3>::TAnimSplineTrack()
{
    AllocSpline();
    m_flags = 0;
    m_defaultValue = Vec3(0, 0, 0);
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
    m_node = nullptr;
    m_trackMultiplier = 1.0f;
}
template <>
inline void TAnimSplineTrack<Vec3>::GetValue(float time, Vec3& value, bool applyMultiplier)
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
inline EAnimCurveType TAnimSplineTrack<Vec3>::GetCurveType() { return eAnimCurveType_TCBVector; }
template <>
inline EAnimValue TAnimSplineTrack<Vec3>::GetValueType() { return eAnimValue_Vector; }
template <>
inline void TAnimSplineTrack<Vec3>::SetValue(float time, const Vec3& value, bool bDefault, bool applyMultiplier)
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
template <>
inline void TAnimSplineTrack<Vec3>::OffsetKeyPosition(const Vec3& offset)
{
    // Iterate over all keys and offet them.
    ITcbKey key;
    for (int i = 0; i < GetNumKeys(); i++)
    {
        // Offset each key.
        GetKey(i, &key);
        key.SetVec3(key.GetVec3() + offset);
        SetKey(i, &key);
    }
}

//////////////////////////////////////////////////////////////////////////
template <>
inline void TAnimSplineTrack<Vec3>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;

    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    sprintf_s(str, "%.2f  %.2f  %.2f", k.value[0], k.value[1], k.value[2]);
}

#endif // CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC3SPECIALIZATION_H
