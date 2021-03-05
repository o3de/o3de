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

// Description : Occlusion Culler using hardware generated ZBuffer


#ifndef CRYINCLUDE_CRY3DENGINE_CZBUFFERCULLER_H
#define CRYINCLUDE_CRY3DENGINE_CZBUFFERCULLER_H
#pragma once

#include <Cry_Math.h>
struct IRenderMesh;

typedef uint16          TZBZexel;
const uint64                TZB_MAXDEPTH    =   (1 << (sizeof(TZBZexel) * 8)) - 1;

class CZBufferCuller
    : public Cry3DEngineBase
{
protected:
    bool           m_DebugFreez;
    uint32         m_SizeX;
    uint32         m_SizeY;
    f32            m_fSizeX;
    f32            m_fSizeY;
    f32            m_fSizeZ;
    TZBZexel*      m_ZBuffer;

    Matrix44       m_MatProj;
    Matrix44       m_MatView;
    Matrix44       m_MatViewProj;
    Matrix44A      m_MatViewProjT;
    Vec3           m_Position;

    int32          m_Bias;
    uint32         m_RotationSafe;
    uint32         m_AccurateTest;
    uint32         m_Treshold;

    f32            m_FixedZFar;
    uint32         m_ObjectsTested;
    uint32         m_ObjectsTestedAndRejected;
    CCamera        m_Camera;
    int            m_OutdoorVisible;

    template<uint32 ROTATE, class T>
    bool                                            Rasterize(const T rVertices, const uint32    VCount)
    {
        int64   MinX = m_SizeX;
        int64   MaxX = 0;
        int64   MinY = m_SizeY;
        int64   MaxY = 0;
        int64 MinZ = TZB_MAXDEPTH;
        for (uint32 a = 0; a < VCount; a++)
        {
            Vec4    V   =   rVertices[a];
            const f32 InvW  =   1.f / V.w;
            int64   X   =   static_cast<int64>((V.x * InvW * 0.5f + 0.5f) * m_fSizeX + 0.5f);
            int64   Y   =   static_cast<int64>((V.y * InvW * 0.5f + 0.5f) * m_fSizeY + 0.5f);
            int64 Z =   static_cast<int64>(V.z * InvW * m_fSizeZ);
            if (X < MinX)
            {
                MinX = X;
            }
            else
            if (X > MaxX)
            {
                MaxX = X;
            }
            if (Y < MinY)
            {
                MinY = Y;
            }
            else
            if (Y > MaxY)
            {
                MaxY = Y;
            }
            if (Z < MinZ)
            {
                MinZ = Z;
            }
        }
        if (MinX < 0)
        {
            if constexpr (ROTATE == 1)
            {
                return true;
            }
            MinX = 0;
        }
        if (MaxX > m_SizeX)
        {
            if constexpr (ROTATE == 1)
            {
                return true;
            }
            MaxX = m_SizeX;
        }
        if (MinY < 0)
        {
            if constexpr (ROTATE == 1)
            {
                return true;
            }
            MinY = 0;
        }
        if (MaxY > m_SizeY)
        {
            if constexpr (ROTATE == 1)
            {
                return true;
            }
            MaxY = m_SizeY;
        }
        if constexpr (ROTATE == 2)
        {
            if (MinX >= m_SizeX || MinY >= m_SizeY || MaxX < 0 ||  MaxX < 0)
            {
                return true;
            }
        }
        for (int64 y = MinY; y < MaxY; y++)
        {
            for (int64 x = MinX; x < MaxX; x++)
            {
                if (static_cast<int64>(m_ZBuffer[static_cast<int32>(x) + static_cast<int32>(y) * m_SizeX]) > MinZ)
                {
                    return true;
                }
            }
        }
        return false;
    }



    bool IsBoxVisible_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_OCEAN(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_OCCELL(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_OCCELL_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_OBJECT(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_OBJECT_TO_LIGHT(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_TERRAIN_NODE(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible_PORTAL(const AABB& objBox, uint32* const __restrict pResDest = NULL);
    bool IsBoxVisible(const AABB& objBox, uint32* const __restrict pResDest = NULL);


public:
    CZBufferCuller();
    ~CZBufferCuller(){CryModuleMemalignFree(m_ZBuffer); }

    // start new frame
    void BeginFrame(const SRenderingPassInfo& passInfo);
    void                                            ReloadBuffer(const uint32 BufferID);

    // render into buffer
    ILINE void                              AddRenderMesh([[maybe_unused]] IRenderMesh* pRM, [[maybe_unused]] Matrix34A* pTranRotMatrix, _smart_ptr<IMaterial> pMaterial, [[maybe_unused]] bool bOutdoorOnly, [[maybe_unused]] bool bCompletelyInFrustum, [[maybe_unused]] bool bNoCull){}


    ILINE bool IsObjectVisible(const AABB& objBox, EOcclusionObjectType eOcclusionObjectType, [[maybe_unused]] float fDistance, uint32* pRetVal = NULL)
    {
        switch (eOcclusionObjectType)
        {
        case eoot_OCCLUDER:
            return IsBoxVisible_OCCLUDER(objBox, pRetVal);
        case eoot_OCEAN:
            return IsBoxVisible_OCEAN(objBox, pRetVal);
        case eoot_OCCELL:
            return IsBoxVisible_OCCELL(objBox, pRetVal);
        case eoot_OCCELL_OCCLUDER:
            return IsBoxVisible_OCCELL_OCCLUDER(objBox, pRetVal);
        case eoot_OBJECT:
            return IsBoxVisible_OBJECT(objBox, pRetVal);
        case eoot_OBJECT_TO_LIGHT:
            return IsBoxVisible_OBJECT_TO_LIGHT(objBox, pRetVal);
        case eoot_TERRAIN_NODE:
            return IsBoxVisible_TERRAIN_NODE(objBox, pRetVal);
        case eoot_PORTAL:
            return IsBoxVisible_PORTAL(objBox, pRetVal);
        }
        assert(!"Undefined occluder type");

        return true;
    }

    // draw content to the screen for debug
    void                                            DrawDebug(int32 nStep);

    // return current camera
    const CCamera&                      GetCamera() const {return m_Camera; }

    void                                            GetMemoryUsage(ICrySizer* pSizer) const;

    bool                                            IsOutdooVisible(){return m_OutdoorVisible == 1; }
    int32                                           TrisWritten() const{return 0; }
    int32                                           ObjectsWritten() const{return 0; }
    int32                                           TrisTested() const{return 0; }
    int32                                           ObjectsTested() const{return m_ObjectsTested; }
    int32                                           ObjectsTestedAndRejected() const{return m_ObjectsTestedAndRejected; }
    int32                                           SelRes() const{return m_SizeX; }
    float                                           FixedZFar() const{return m_FixedZFar; }
    float                                           GetZNearInMeters() const{return 0.f; }
    float                                           GetZFarInMeters() const{return 1024; }
} _ALIGN(128);

ILINE bool CZBufferCuller::IsBoxVisible_TERRAIN_NODE(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCELL_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCLUDER(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCEAN(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OCCELL(const AABB& objBox, uint32* const __restrict pResDest)
{
    if (GetCVars()->e_CoverageBufferDebugFreeze)
    {
        return true;
    }
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OBJECT(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_OBJECT_TO_LIGHT(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

ILINE bool CZBufferCuller::IsBoxVisible_PORTAL(const AABB& objBox, uint32* const __restrict pResDest)
{
    return IsBoxVisible(objBox, pResDest);
}

#endif // CRYINCLUDE_CRY3DENGINE_CZBUFFERCULLER_H
