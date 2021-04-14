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

// Description : Occlusion (coverage) buffer

#include "Cry3DEngine_precompiled.h"
#include "cbuffer.h"
#include "StatObj.h"

class CCoverageBuffer
    : public Cry3DEngineBase
{
public:
    // can be used by other classes
    static void ClipPolygon(PodArray<Vec3>& PolygonOut, const PodArray<Vec3>& pPolygon, const Plane& ClipPlane);
    static void ClipPolygon(PodArray<Vec3>* pPolygon, const Plane& ClipPlane);
    static void MatMul4(float* product, const float* a, const float* b);
    static void TransformPoint(float out[4], const float m[16], const float in[4]);

protected:

    static int ClipEdge(const Vec3& v1, const Vec3& v2, const Plane& ClipPlane, Vec3& vRes1, Vec3& vRes2);

#if defined(WIN32) && defined(_CPU_X86)
    inline int fastfround(float f) // note: only positive numbers works correct
    {
        int i;
        __asm fld[f]
            __asm fistp[i]
            return i;
    }
#else
    inline int fastfround(float f) { int i; i = (int)(f + 0.5f); return i; } // note: only positive numbers works correct
#endif
};

void CCoverageBuffer::TransformPoint(float out[4], const float m[16], const float in[4])
{
#define M(row, col)  m[col * 4 + row]
    out[0] = M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
    out[1] = M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
    out[2] = M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
    out[3] = M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

void CCoverageBuffer::MatMul4(float* product, const float* a, const float* b)
{
#define A(row, col)  a[(col << 2) + row]
#define B(row, col)  b[(col << 2) + row]
#define P(row, col)  product[(col << 2) + row]
    int i;
    for (i = 0; i < 4; i++)
    {
        float ai0 = A(i, 0),  ai1 = A(i, 1),  ai2 = A(i, 2),  ai3 = A(i, 3);
        P(i, 0) = ai0 * B(0, 0) + ai1 * B(1, 0) + ai2 * B(2, 0) + ai3 * B(3, 0);
        P(i, 1) = ai0 * B(0, 1) + ai1 * B(1, 1) + ai2 * B(2, 1) + ai3 * B(3, 1);
        P(i, 2) = ai0 * B(0, 2) + ai1 * B(1, 2) + ai2 * B(2, 2) + ai3 * B(3, 2);
        P(i, 3) = ai0 * B(0, 3) + ai1 * B(1, 3) + ai2 * B(2, 3) + ai3 * B(3, 3);
    }
#undef A
#undef B
#undef P
}

// return number of vertices to add
int CCoverageBuffer::ClipEdge(const Vec3& v1, const Vec3& v2, const Plane& ClipPlane, Vec3& vRes1, Vec3& vRes2)
{
    float d1 = -ClipPlane.DistFromPlane(v1);
    float d2 = -ClipPlane.DistFromPlane(v2);
    if (d1 < 0 && d2 < 0)
    {
        return 0; // all clipped = do not add any vertices
    }
    if (d1 >= 0 && d2 >= 0)
    {
        vRes1 = v2;
        return 1; // both not clipped - add second vertex
    }

    // calculate new vertex
    Vec3 vIntersectionPoint = v1 + (v2 - v1) * (Ffabs(d1) / (Ffabs(d2) + Ffabs(d1)));

#ifdef _DEBUG
    float fNewDist = -ClipPlane.DistFromPlane(vIntersectionPoint);
    assert(Ffabs(fNewDist) < 0.01f);
#endif

    if (d1 >= 0 && d2 < 0)
    { // from vis to no vis
        vRes1 = vIntersectionPoint;
        return 1;
    }
    else if (d1 < 0 && d2 >= 0)
    { // from not vis to vis
        vRes1 = vIntersectionPoint;
        vRes2 = v2;
        return 2;
    }

    assert(0);
    return 0;
}

void CCoverageBuffer::ClipPolygon(PodArray<Vec3>& PolygonOut, const PodArray<Vec3>& pPolygon, const Plane& ClipPlane)
{
    PolygonOut.Clear();
    // clip edges, make list of new vertices
    for (int i = 0; i < pPolygon.Count(); i++)
    {
        Vec3 vNewVert1(0, 0, 0), vNewVert2(0, 0, 0);
        if (int nNewVertNum = ClipEdge(pPolygon.GetAt(i), pPolygon.GetAt((i + 1) % pPolygon.Count()), ClipPlane, vNewVert1, vNewVert2))
        {
            PolygonOut.Add(vNewVert1);
            if (nNewVertNum > 1)
            {
                PolygonOut.Add(vNewVert2);
            }
        }
    }

    // check result
#if !defined(NDEBUG)
    for (int i = 0; i < PolygonOut.Count(); i++)
    {
        float d1 = -ClipPlane.DistFromPlane(PolygonOut.GetAt(i));           
        assert(d1 >= -0.01f);
    }
#endif

    assert(PolygonOut.Count() == 0 || PolygonOut.Count() >= 3);
}

void CCoverageBuffer::ClipPolygon(PodArray<Vec3>* pPolygon, const Plane& ClipPlane)
{
    static PodArray<Vec3> PolygonOut; // Keep this list static to not perform reallocation every time.
    PolygonOut.Clear();
    ClipPolygon(*pPolygon, PolygonOut, ClipPlane);
    pPolygon->Clear();
    pPolygon->AddList(PolygonOut);
}

bool IsABBBVisibleInFrontOfPlane_FAST(const AABB& objBox, const SPlaneObject& clipPlane)
{
    const f32* p = &objBox.min.x;
    if ((clipPlane.plane | Vec3(p[clipPlane.vIdx2.x], p[clipPlane.vIdx2.y], p[clipPlane.vIdx2.z])) > 0)
    {
        return true;
    }

    return false;
}

CPolygonClipContext::CPolygonClipContext()
{
}

void CPolygonClipContext::Reset()
{
    stl::free_container(m_lstPolygonA);
    stl::free_container(m_lstPolygonB);
}

const PodArray<Vec3>& CPolygonClipContext::Clip(const PodArray<Vec3>& poly, const Plane* planes, size_t numPlanes)
{
    m_lstPolygonA.Clear();
    m_lstPolygonB.Clear();

    m_lstPolygonA.AddList(poly);

    PodArray<Vec3>* src = &m_lstPolygonA, * dst = &m_lstPolygonB;

    for (size_t i = 0; i < numPlanes && src->Count() >= 3; std::swap(src, dst), i++)
    {
        CCoverageBuffer::ClipPolygon(*dst, *src, planes[i]);
    }

    return *src;
}

const PodArray<Vec3>& CPolygonClipContext::Clip(const Vec3& a, const Vec3& b, const Vec3& c, const Plane* planes, size_t numPlanes)
{
    m_lstPolygonA.Clear();
    m_lstPolygonB.Clear();

    m_lstPolygonA.Add(a);
    m_lstPolygonA.Add(b);
    m_lstPolygonA.Add(c);

    PodArray<Vec3>* src = &m_lstPolygonA, * dst = &m_lstPolygonB;

    for (size_t i = 0; i < numPlanes && src->Count() >= 3; std::swap(src, dst), i++)
    {
        CCoverageBuffer::ClipPolygon(*dst, *src, planes[i]);
    }

    return *src;
}

void CPolygonClipContext::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_lstPolygonA);
    pSizer->AddObject(m_lstPolygonB);
}
