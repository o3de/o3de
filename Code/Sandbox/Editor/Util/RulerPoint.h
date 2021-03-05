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

// Description : Ruler point helper, used by CRuler


#ifndef CRYINCLUDE_EDITOR_UTIL_RULERPOINT_H
#define CRYINCLUDE_EDITOR_UTIL_RULERPOINT_H
#pragma once


class CRuler;

//! Ruler point helper - Defines a point for the ruler
class CRulerPoint
{
public:
    CRulerPoint();
    CRulerPoint& operator =(CRulerPoint const& other);

    void Reset();
    void Render(IRenderer* pRenderer);

    //! Set helpers
    void Set(const Vec3& vPos);
    void Set(CBaseObject* pObject);
    void SetHelperSettings(float scale, float trans);

    //! Returns is point has valid data in it (in use)
    bool IsEmpty() const;

    //! Helpers to get correct data out
    Vec3 GetPos() const;
    Vec3 GetMidPoint(const CRulerPoint& otherPoint) const;
    float GetDistance(const CRulerPoint& otherPoint) const;
    CBaseObject* GetObject() const;

private:
    enum EType
    {
        eType_Invalid,
        eType_Point,
        eType_Object,

        eType_COUNT,
    };
    EType m_type;

    Vec3 m_vPoint;
    GUID m_ObjectGUID;
    float m_sphereScale;
    float m_sphereTrans;
};

#endif // CRYINCLUDE_EDITOR_UTIL_RULERPOINT_H
