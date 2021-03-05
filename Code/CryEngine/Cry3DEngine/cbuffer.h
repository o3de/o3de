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

#ifndef CRYINCLUDE_CRY3DENGINE_CBUFFER_H
#define CRYINCLUDE_CRY3DENGINE_CBUFFER_H
#pragma once

#include <AzCore/Casting/numeric_cast.h>

#define Point2d Vec3

struct SPlaneObject
{
    Plane plane;
    Vec3_tpl<uint8> vIdx1, vIdx2;

    void Update()
    {
        //avoid breaking strict aliasing rules
        union f32_u
        {
            float floatVal;
            uint32 uintVal;
        };
        f32_u ux;
        ux.floatVal = plane.n.x;
        f32_u uy;
        uy.floatVal = plane.n.y;
        f32_u uz;
        uz.floatVal = plane.n.z;
        uint32 bitX =   ux.uintVal >> 31;
        uint32 bitY =   uy.uintVal >> 31;
        uint32 bitZ =   uz.uintVal >> 31;
        vIdx1.x = aznumeric_caster(bitX * 3 + 0);
        vIdx2.x = aznumeric_caster((1 - bitX) * 3 + 0);
        vIdx1.y = aznumeric_caster(bitY * 3 + 1);
        vIdx2.y = aznumeric_caster((1 - bitY) * 3 + 1);
        vIdx1.z = aznumeric_caster(bitZ * 3 + 2);
        vIdx2.z = aznumeric_caster((1 - bitZ) * 3 + 2);
    }
};

class CPolygonClipContext
{
public:
    CPolygonClipContext();

    void Reset();

    const PodArray<Vec3>& Clip(const PodArray<Vec3>& poly, const Plane* planes, size_t numPlanes);
    const PodArray<Vec3>& Clip(const Vec3& a, const Vec3& b, const Vec3& c, const Plane* planes, size_t numPlanes);

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    PodArray<Vec3> m_lstPolygonA;
    PodArray<Vec3> m_lstPolygonB;
};

#endif // CRYINCLUDE_CRY3DENGINE_CBUFFER_H
