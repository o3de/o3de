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

// Description : Facility for efficiently generating random positions on geometry

#ifndef CRYINCLUDE_CRYCOMMON_GEOMQUERY_H
#define CRYINCLUDE_CRYCOMMON_GEOMQUERY_H
#pragma once


#include "Cry_Geo.h"
#include "CryArray.h"
#include "Random.h"

//////////////////////////////////////////////////////////////////////
// Extents cache

class CGeomExtent
{
public:

    CGeomExtent()
        : m_nEmptyEndParts(0) {}

    ILINE operator bool() const
    {
        return m_afCumExtents.capacity() + m_nEmptyEndParts != 0;
    }
    ILINE int NumParts() const
    {
        return m_afCumExtents.size();
    }
    ILINE float TotalExtent() const
    {
        return !m_afCumExtents.empty() ? m_afCumExtents.back() : 0.f;
    }

    void Clear()
    {
        m_afCumExtents.clear();
        m_nEmptyEndParts = 0;
    }
    void AddPart(float fExtent)
    {
        // Defer empty parts until a non-empty part is added.
        if (fExtent <= 0.f)
        {
            m_nEmptyEndParts++;
        }
        else
        {
            float fTotal = TotalExtent();
            for (; m_nEmptyEndParts; m_nEmptyEndParts--)
            {
                m_afCumExtents.push_back(fTotal);
            }
            m_afCumExtents.push_back(fTotal + fExtent);
        }
    }
    void ReserveParts(int nCount)
    {
        m_afCumExtents.reserve(nCount);
    }

    // Find element in sorted array <= index (normalized 0 to 1)
    int GetPart(float fIndex) const
    {
        int last = m_afCumExtents.size() - 1;
        if (last <= 0)
        {
            return last;
        }

        fIndex *= m_afCumExtents[last];

        // Binary search thru array.
        int lo = 0, hi = last;
        while (lo < hi)
        {
            int i = (lo + hi) >> 1;
            if (fIndex < m_afCumExtents[i])
            {
                hi = i;
            }
            else
            {
                lo = i + 1;
            }
        }

        assert(lo == 0 || m_afCumExtents[lo] > m_afCumExtents[lo - 1]);
        return lo;
    }

    int RandomPart() const
    {
        return GetPart(cry_random(0.0f, 1.0f));
    }

protected:

    DynArray<float> m_afCumExtents;
    int m_nEmptyEndParts;
};

class CGeomExtents
{
public:

    ILINE CGeomExtents()
        : m_aExtents(0) {}
    ~CGeomExtents()
    { delete[] m_aExtents; }

    void Clear()
    {
        delete[] m_aExtents;
        m_aExtents = 0;
    }

    ILINE CGeomExtent const& operator [](EGeomForm eForm) const
    {
        assert(eForm >= 0 && eForm < MaxGeomForm);
        if (m_aExtents)
        {
            return m_aExtents[eForm];
        }

        static CGeomExtent s_empty;
        return s_empty;
    }

    ILINE CGeomExtent& Make(EGeomForm eForm)
    {
        assert(eForm >= 0 && eForm < MaxGeomForm);
        if (!m_aExtents)
        {
            m_aExtents = new CGeomExtent[4];
        }
        return m_aExtents[eForm];
    }

protected:
    CGeomExtent* m_aExtents;
};


// Other random/extent functions

inline float ScaleExtent(EGeomForm eForm, float fScale)
{
    switch (eForm)
    {
    default:
        return 1;
    case GeomForm_Edges:
        return fScale;
    case GeomForm_Surface:
        return fScale * fScale;
    case GeomForm_Volume:
        return fScale * fScale * fScale;
    }
}

inline float BoxExtent(EGeomForm eForm, Vec3 const& vSize)
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:
        return 8.f;
    case GeomForm_Edges:
        return (vSize.x + vSize.y + vSize.z) * 8.f;
    case GeomForm_Surface:
        return (vSize.x * vSize.y + vSize.x * vSize.z + vSize.y * vSize.z) * 8.f;
    case GeomForm_Volume:
        return vSize.x * vSize.y * vSize.z * 8.f;
    }
}

// Utility functions.

template<class T>
inline
const typename T::value_type& RandomElem(const T& array)
{
    int n = cry_random(0U, array.size() - 1);
    return array[n];
}

// Geometric primitive randomizing functions.
ILINE void BoxRandomPos(PosNorm& ran, EGeomForm eForm, Vec3 const& vSize)
{
    ran.vPos = cry_random_componentwise(-vSize, vSize);
    ran.vNorm = ran.vPos;

    if (eForm != GeomForm_Volume)
    {
        // Generate a random corner, for collapsing random point.
        int nCorner = cry_random(0, 7);
        ran.vNorm.x = (((nCorner & 1) << 1) - 1) * vSize.x;
        ran.vNorm.y = (((nCorner & 2))   - 1) * vSize.y;
        ran.vNorm.z = (((nCorner & 4) >> 1) - 1) * vSize.z;

        if (eForm == GeomForm_Vertices)
        {
            ran.vPos = ran.vNorm;
        }
        else if (eForm == GeomForm_Surface)
        {
            // Collapse one axis.
            float fAxis = cry_random(0.0f, vSize.x * vSize.y + vSize.y * vSize.z + vSize.z * vSize.x);
            if ((fAxis -= vSize.y * vSize.z) < 0.f)
            {
                ran.vPos.x = ran.vNorm.x;
                ran.vNorm.y = ran.vNorm.z = 0.f;
            }
            else if ((fAxis -= vSize.z * vSize.x) < 0.f)
            {
                ran.vPos.y = ran.vNorm.y;
                ran.vNorm.x = ran.vNorm.z = 0.f;
            }
            else
            {
                ran.vPos.z = ran.vNorm.z;
                ran.vNorm.x = ran.vNorm.y = 0.f;
            }
        }
        else if (eForm == GeomForm_Edges)
        {
            // Collapse 2 axes.
            float fAxis = cry_random(0.0f, vSize.x + vSize.y + vSize.z);
            if ((fAxis -= vSize.x) < 0.f)
            {
                ran.vPos.y = ran.vNorm.y;
                ran.vPos.z = ran.vNorm.z;
                ran.vNorm.x = 0.f;
            }
            else if ((fAxis -= vSize.y) < 0.f)
            {
                ran.vPos.x = ran.vNorm.x;
                ran.vPos.z = ran.vNorm.z;
                ran.vNorm.y = 0.f;
            }
            else
            {
                ran.vPos.x = ran.vNorm.x;
                ran.vPos.y = ran.vNorm.y;
                ran.vNorm.z = 0.f;
            }
        }
    }

    ran.vNorm.Normalize();
}

inline float CircleExtent(EGeomForm eForm, float fRadius)
{
    switch (eForm)
    {
    case GeomForm_Edges:
        return gf_PI2 * fRadius;
    case GeomForm_Surface:
        return gf_PI * square(fRadius);
    default:
        return 1.f;
    }
}

inline Vec2 CircleRandomPoint(EGeomForm eForm, float fRadius)
{
    Vec2 vPt;
    switch (eForm)
    {
    case GeomForm_Edges:
        // Generate random angle.
        sincos_tpl(cry_random(0.0f, gf_PI2), &vPt.y, &vPt.x);
        vPt *= fRadius;
        break;
    case GeomForm_Surface:
        // Generate random angle, and radius, adjusted for even distribution.
        sincos_tpl(cry_random(0.0f, gf_PI2), &vPt.y, &vPt.x);
        vPt *= sqrt(cry_random(0.0f, 1.0f)) * fRadius;
        break;
    default:
        vPt.x = vPt.y = 0.f;
    }
    return vPt;
}

inline float SphereExtent(EGeomForm eForm, float fRadius)
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:
    case GeomForm_Edges:
        return 0.f;
    case GeomForm_Surface:
        return gf_PI * 4.f * sqr(fRadius);
    case GeomForm_Volume:
        return gf_PI * 4.f / 3.f * cube(fRadius);
    }
}

inline void SphereRandomPos(PosNorm& ran, EGeomForm eForm, float fRadius)
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:
    case GeomForm_Edges:
        ran.vPos.zero();
        ran.vNorm.zero();
        return;
    case GeomForm_Surface:
    case GeomForm_Volume:
    {
        // Generate point on surface, as normal.
        float fPhi = cry_random(0.0f, gf_PI2);
        float fZ = cry_random(-1.f, 1.f);
        float fH = sqrt_tpl(1.f - fZ * fZ);
        sincos_tpl(fPhi, &ran.vNorm.y, &ran.vNorm.x);
        ran.vNorm.x *= fH;
        ran.vNorm.y *= fH;
        ran.vNorm.z = fZ;

        ran.vPos = ran.vNorm;
        if (eForm == GeomForm_Volume)
        {
            float fV = cry_random(0.0f, 1.0f);
            float fR = pow_tpl(fV, 0.333333f);
            ran.vPos *= fR;
        }
        ran.vPos *= fRadius;
        break;
    }
    }
}

// Triangle randomisation functions

inline float TriExtent(EGeomForm eForm, Vec3 const aPos[3])
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Edges:
        return (aPos[1] - aPos[0]).GetLengthFast();
    case GeomForm_Surface:
        return ((aPos[1] - aPos[0]) % (aPos[2] - aPos[0])).GetLengthFast() * 0.5f;
    case GeomForm_Volume:
        // Generate signed volume of pyramid by computing triple product of vertices.
        return ((aPos[0] ^ aPos[1]) | aPos[2]) / 6.0f;
    }
}

inline void TriRandomPos(PosNorm& ran, EGeomForm eForm, PosNorm const aRan[3], bool bDoNormals)
{
    // Generate interpolators for verts.
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:
        ran = aRan[0];
        return;
    case GeomForm_Edges:
    {
        float t = cry_random(0.0f, 1.0f);
        ran.vPos = aRan[0].vPos * (1.f - t) + aRan[1].vPos * t;
        if (bDoNormals)
        {
            ran.vNorm = aRan[0].vNorm * (1.f - t) + aRan[1].vNorm * t;
        }
        break;
    }
    case GeomForm_Surface:
    {
        float t0 = cry_random(0.0f, 1.0f);
        float t1 = cry_random(0.0f, 1.0f);
        float t2 = cry_random(0.0f, 1.0f);
        float fSum = t0 + t1 + t2;
        ran.vPos = (aRan[0].vPos * t0 + aRan[1].vPos * t1 + aRan[2].vPos * t2) * (1.f / fSum);
        if (bDoNormals)
        {
            ran.vNorm = aRan[0].vNorm * t0 + aRan[1].vNorm * t1 + aRan[2].vNorm * t2;
        }
        break;
    }
    case GeomForm_Volume:
    {
        float t0 = cry_random(0.0f, 1.0f);
        float t1 = cry_random(0.0f, 1.0f);
        float t2 = cry_random(0.0f, 1.0f);
        float t3 = cry_random(0.0f, 1.0f);
        float fSum = t0 + t1 + t2 + t3;
        ran.vPos = (aRan[0].vPos * t0 + aRan[1].vPos * t1 + aRan[2].vPos * t2) * (1.f / fSum);
        if (bDoNormals)
        {
            ran.vNorm = (aRan[0].vNorm * t0 + aRan[1].vNorm * t1 + aRan[2].vNorm * t2) * (1.f - t3) + ran.vPos.GetNormalizedFast() * t3;
        }
        break;
    }
    }
    if (bDoNormals)
    {
        ran.vNorm.Normalize();
    }
}

// Mesh random pos functions

inline int TriMeshPartCount(EGeomForm eForm, int nIndices)
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:
    case GeomForm_Edges:
        // Number of edges = verts.
        return nIndices;
    case GeomForm_Surface:
    case GeomForm_Volume:
        // Number of tris.
        assert(nIndices % 3 == 0);
        return nIndices / 3;
    }
}

inline int TriIndices(int aIndices[3], int nPart, EGeomForm eForm)
{
    switch (eForm)
    {
    default:
        assert(0);
    case GeomForm_Vertices:             // Part is vert index
        aIndices[0] = nPart;
        return 1;
    case GeomForm_Edges:                    // Part is vert index
        aIndices[0] = nPart;
        aIndices[1] = nPart % 3 < 2 ? nPart + 1 : nPart - 2;
        return 2;
    case GeomForm_Surface:              // Part is tri index
    case GeomForm_Volume:
        aIndices[0] = nPart * 3;
        aIndices[1] = aIndices[0] + 1;
        aIndices[2] = aIndices[0] + 2;
        return 3;
    }
}


#endif // CRYINCLUDE_CRYCOMMON_GEOMQUERY_H
