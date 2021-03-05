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

#ifndef CRYINCLUDE_CRY3DENGINE_RENDERMESHUTILS_H
#define CRYINCLUDE_CRY3DENGINE_RENDERMESHUTILS_H
#pragma once

struct SIntersectionData;

//////////////////////////////////////////////////////////////////////////
// RenderMesh utilities.
//////////////////////////////////////////////////////////////////////////
class CRenderMeshUtils
    : public Cry3DEngineBase
{
public:
    // Do a render-mesh vs Ray intersection, return true for intersection.
    static bool RayIntersection(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pMtl = 0);
    static bool RayIntersectionFast(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0);

    // async versions, aren't using the cache, and are used by the deferredrayintersection class
    static void RayIntersectionAsync(SIntersectionData* pIntersectionRMData);

    static void ClearHitCache();


    //////////////////////////////////////////////////////////////////////////
    //void FindCollisionWithRenderMesh( IRenderMesh *pRenderMesh, SRayHitInfo &hitInfo );
    //  void FindCollisionWithRenderMesh( IPhysiIRenderMesh2 *pRenderMesh, SRayHitInfo &hitInfo );
private:
    // functions implementing the logic for RayIntersection
    static bool RayIntersectionImpl(SIntersectionData* pIntersectionRMData, SRayHitInfo* phitInfo, _smart_ptr<IMaterial> pCustomMtl, bool bAsync);
    static bool RayIntersectionFastImpl(SIntersectionData& rIntersectionRMData, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl, bool bAsync);
    static bool ProcessBoxIntersection(Ray& inRay, SRayHitInfo& hitInfo, SIntersectionData& rIntersectionRMData, _smart_ptr<IMaterial> pMtl, vtx_idx* pInds, int nVerts, uint8* pPos, int nPosStride, uint8* pUV, int nUVStride, uint8* pCol, int nColStride, int nInds, bool& bAnyHit, float& fBestDist, Vec3& vHitPos, Vec3* tri);
};

// struct to collect parameters for the wrapped RayInterseciton functions
struct SIntersectionData
{
    SIntersectionData()
        : pRenderMesh(NULL)
        ,  nVerts(0)
        ,  nInds(0)
        , nPosStride(0)
        ,          pPos(NULL)
        , pInds(NULL)
        , nUVStride(0)
        ,           pUV(NULL)
        , nColStride(0)
        ,          pCol(NULL)
        , nTangsStride(0)
        ,        pTangs(NULL)
        , bResult(false)
        ,         bNeedFallback(false)
        , fDecalPlacementTestMaxSize(1000.f)
        , bDecalPlacementTestRequested(false)
        , pHitInfo(0)
        , pMtl(0)
    {
    }

    bool Init(IRenderMesh* pRenderMesh, SRayHitInfo* _pHitInfo, _smart_ptr<IMaterial> _pMtl, bool _bRequestDecalPlacementTest = false);

    IRenderMesh* pRenderMesh;
    SRayHitInfo* pHitInfo;
    _smart_ptr<IMaterial> pMtl;
    bool bDecalPlacementTestRequested;

    int nVerts;
    int nInds;

    int nPosStride;
    uint8* pPos;
    vtx_idx* pInds;

    int nUVStride;
    uint8* pUV;

    int nColStride;
    uint8* pCol;

    int nTangsStride;
    byte* pTangs;

    bool bResult;
    float fDecalPlacementTestMaxSize; // decal will look acceptable in this place
    bool bNeedFallback;
};

template <class T>
bool GetBarycentricCoordinates(const T& a, const T& b, const T& c, const T& p, float& u, float& v, float& w, float fBorder = 0.f);

#endif // CRYINCLUDE_CRY3DENGINE_RENDERMESHUTILS_H
