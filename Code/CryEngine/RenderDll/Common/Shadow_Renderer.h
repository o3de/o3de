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
#if !defined(SHADOWRENDERER_H)
#define SHADOWRENDERER_H


#include "ShadowUtils.h"
#include <IRenderAuxGeom.h>
#include <VectorSet.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>

#define OMNI_SIDES_NUM 6

// data used to compute a custom shadow frustum for near shadows
struct CustomShadowMapFrustumData
{
    AABB aabb;
};

struct ShadowMapFrustum
{
    enum eFrustumType // NOTE: Be careful when modifying the enum as it is used for sorting frustums in SCompareByLightIds
    {
        e_GsmDynamic         = 0,
        e_GsmDynamicDistance = 1,
        e_GsmCached          = 2,
        e_HeightMapAO        = 3,
        e_Nearest            = 4,
        e_PerObject          = 5,

        e_NumTypes
    };

    eFrustumType m_eFrustumType;

    Matrix44A mLightProjMatrix;
    Matrix44A mLightViewMatrix;

    // flags
    bool    bUseAdditiveBlending;
    bool    bIncrementalUpdate;

    // if set to true - m_castersList contains all casters in light radius
    // and all other members related only to single frustum projection case are undefined
    bool    bOmniDirectionalShadow;
    uint8   nOmniFrustumMask;
    uint8   nInvalidatedFrustMask[MAX_GPU_NUM]; //for each GPU
    bool    bBlendFrustum;
    float fBlendVal;

    uint32 nShadowGenMask;
    bool    bIsMGPUCopy;

    bool bHWPCFCompare;

    uint8 nShadowPoolUpdateRate;

    //sampling parameters
    f32 fWidthS, fWidthT;
    f32 fBlurS, fBlurT;

    //fading distance per light source
    float fShadowFadingDist;

    ETEX_Format             m_eReqTF;
    ETEX_Type               m_eReqTT;

    //texture in pool
    bool bUseShadowsPool;
    struct ShadowMapFrustum* pPrevFrustum = nullptr;
    struct ShadowMapFrustum* pFrustumOwner = nullptr;
    class CTexture* pDepthTex;

    //3d engine parameters
    float fFOV;
    float fNearDist;
    float fFarDist;
    int   nTexSize;

    //shadow renderer parameters - should be in separate structure
    //atlas parameters
    int   nTextureWidth;
    int   nTextureHeight;
    bool  bUnwrapedOmniDirectional;
    int   nShadowMapSize;

    //packer params
    uint nPackID[OMNI_SIDES_NUM];
    int packX[OMNI_SIDES_NUM];
    int packY[OMNI_SIDES_NUM];
    int packWidth[OMNI_SIDES_NUM];
    int packHeight[OMNI_SIDES_NUM];

    int   nResetID;
    float fFrustrumSize;
    float fProjRatio;
    float fDepthTestBias;
    float fDepthConstBias;
    float fDepthSlopeBias;
    PodArray<struct IShadowCaster*> m_castersList;
    PodArray<struct IShadowCaster*> m_jobExecutedCastersList;

    CCamera FrustumPlanes[OMNI_SIDES_NUM];
    uint32 nShadowGenID[RT_COMMAND_BUF_COUNT][OMNI_SIDES_NUM];
    AABB aabbCasters; //casters bbox in world space
    Vec3 vLightSrcRelPos; // relative world space
    Vec3 vProjTranslation; // dst position
    float fRadius;
    int nUpdateFrameId;
    IRenderNode* pLightOwner = nullptr;
    uint32 uCastersListCheckSum;
    int nShadowMapLod;          // currently use as GSMLod, can be used as cubemap side, -1 means this variable is not used

    uint32  m_Flags;

    struct ShadowCacheData
    {
        enum eUpdateStrategy
        {
            // Renders the entire cached shadowmap in one pass.
            // Generally used for a single frame when an event (script, level load, proximity to frustum border, etc.) requests an update of the cache.
            // Will revert to one of the other methods after the update occurs.
            eFullUpdate,

            // Cached shadow frustums will constantly check if updates are required due to moving objects or proximity to the frustum border.
            // Has potentially very high CPU overhead because each cached shadow map frustum culls the octree each frame.
            // Potentially higher GPU overhead because may render extra dynamic or distant objects each frame.
            eIncrementalUpdate, 

            // Updates must triggered manually via script.  Most optimal solution, but requires manual setup.
            eManualUpdate, 

            // Updates may either be triggered manually by script or when the camera moves too close to the border of the shadow frustum
            eManualOrDistanceUpdate 
        };

        ShadowCacheData() { Reset(); }

        void Reset()
        {
            memset(mOctreePath, 0x0, sizeof(mOctreePath));
            memset(mOctreePathNodeProcessed, 0x0, sizeof(mOctreePathNodeProcessed));
            mProcessedCasters.clear();
            mProcessedTerrainCasters.clear();
        }

        static const int MAX_TRAVERSAL_PATH_LENGTH = 32;
        uint8 mOctreePath[MAX_TRAVERSAL_PATH_LENGTH];
        uint8 mOctreePathNodeProcessed[MAX_TRAVERSAL_PATH_LENGTH];

        VectorSet<struct IShadowCaster*> mProcessedCasters;
        VectorSet<uint64> mProcessedTerrainCasters;
    }* pShadowCacheData = nullptr;


    ShadowMapFrustum()
    {
        ZeroStruct(*this);
        fProjRatio = 1.f;

        //initial frustum position should be outside of the visible map
        vProjTranslation = Vec3(-1000.0f, -1000.0f, -1000.0f);

        bUnwrapedOmniDirectional = false;
        nShadowMapSize = 0;

        nUpdateFrameId = -1000;

        pPrevFrustum = nullptr;
        aabbCasters.Reset();
    }

    ShadowMapFrustum(const ShadowMapFrustum& rOther)
    {
        (*this) = rOther;
    }

    ~ShadowMapFrustum()
    {
        // make sure that the renderer isn't using this shadow frustum anymore
        threadID nThreadID = 0;
        gEnv->pRenderer->EF_Query(EFQ_RenderThreadList, nThreadID);
        gEnv->pRenderer->GetFinalizeShadowRendItemJobExecutor(nThreadID)->WaitForCompletion();

        SAFE_DELETE(pShadowCacheData);
    }

    ShadowMapFrustum& operator=(const ShadowMapFrustum& rOther)
    {
        m_eFrustumType = rOther.m_eFrustumType;
        mLightProjMatrix = rOther.mLightProjMatrix;
        mLightViewMatrix = rOther.mLightViewMatrix;

        bUseAdditiveBlending = rOther.bUseAdditiveBlending;
        bOmniDirectionalShadow = rOther.bOmniDirectionalShadow;
        bBlendFrustum = rOther.bBlendFrustum;
        fBlendVal = rOther.fBlendVal;
        bIncrementalUpdate = rOther.bIncrementalUpdate;
        nOmniFrustumMask = rOther.nOmniFrustumMask;
        memcpy(nInvalidatedFrustMask, rOther.nInvalidatedFrustMask, sizeof(nInvalidatedFrustMask));
        nShadowGenMask = rOther.nShadowGenMask;

        bHWPCFCompare = rOther.bHWPCFCompare;

        nShadowPoolUpdateRate = rOther.nShadowPoolUpdateRate;

        fWidthS = rOther.fWidthS;
        fWidthT = rOther.fWidthT;
        fBlurS = rOther.fBlurS;
        fBlurT = rOther.fBlurT;

        fShadowFadingDist = rOther.fShadowFadingDist;

        m_eReqTF = rOther.m_eReqTF;
        m_eReqTT = rOther.m_eReqTT;

        bUseShadowsPool = rOther.bUseShadowsPool;

        // those pointer are not owned by the shadowfrustum, so we don't need a deep copy
        pFrustumOwner = rOther.pFrustumOwner;
        pDepthTex = rOther.pDepthTex;
        pLightOwner = rOther.pLightOwner;

        fFOV = rOther.fFOV;
        fNearDist = rOther.fNearDist;
        fFarDist = rOther.fFarDist;
        nTexSize = rOther.nTexSize;

        nTextureWidth = rOther.nTextureWidth;
        nTextureHeight = rOther.nTextureHeight;
        bUnwrapedOmniDirectional = rOther.bUnwrapedOmniDirectional;
        nShadowMapSize = rOther.nShadowMapSize;

        memcpy(nPackID, rOther.nPackID, sizeof(nPackID));
        memcpy(packX, rOther.packX, sizeof(packX));
        memcpy(packY, rOther.packY, sizeof(packY));
        memcpy(packWidth, rOther.packWidth, sizeof(packWidth));
        memcpy(packHeight, rOther.packHeight, sizeof(packHeight));

        nResetID = rOther.nResetID;
        fFrustrumSize = rOther.fFrustrumSize;
        fProjRatio = rOther.fProjRatio;
        fDepthTestBias = rOther.fDepthTestBias;
        fDepthConstBias = rOther.fDepthConstBias;
        fDepthSlopeBias = rOther.fDepthSlopeBias;

        m_castersList = rOther.m_castersList;
        m_jobExecutedCastersList = rOther.m_jobExecutedCastersList;

        memcpy(FrustumPlanes, rOther.FrustumPlanes, sizeof(FrustumPlanes));
        memcpy(nShadowGenID, rOther.nShadowGenID, sizeof(nShadowGenID));

        aabbCasters = rOther.aabbCasters;
        vLightSrcRelPos = rOther.vLightSrcRelPos;
        vProjTranslation = rOther.vProjTranslation;
        fRadius = rOther.fRadius;
        nUpdateFrameId = rOther.nUpdateFrameId;

        uCastersListCheckSum = rOther.uCastersListCheckSum;
        nShadowMapLod = rOther.nShadowMapLod;
        m_Flags = rOther.m_Flags;

        bIsMGPUCopy = rOther.bIsMGPUCopy;
        if (rOther.pShadowCacheData != nullptr)
        {
            if (pShadowCacheData == nullptr)
            {
                pShadowCacheData = new ShadowCacheData();
            }
            *pShadowCacheData = *rOther.pShadowCacheData;
        }
        else
        {
            SAFE_DELETE(pShadowCacheData);
        }

        return *this;
    }

    void GetSideViewport(int nSide, int* pViewport)
    {
        if (bUseShadowsPool)
        {
            pViewport[0] = packX[nSide];
            pViewport[1] = packY[nSide];
            pViewport[2] = packWidth[nSide];
            pViewport[3] = packHeight[nSide];
        }
        else
        {
            //simplest cubemap 6 faces unwrap
            pViewport[0] = nShadowMapSize * (nSide % 3);
            pViewport[1] = nShadowMapSize * (nSide / 3);
            pViewport[2] = nShadowMapSize;
            pViewport[3] = nShadowMapSize;
        }
    }

    void GetTexOffset(int nSide, float* pOffset, float* pScale, int nShadowsPoolSizeX, int nShadowsPoolSizeY)
    {
        if (bUseShadowsPool)
        {
            pScale[0] = float(nShadowMapSize) / nShadowsPoolSizeX; //SHADOWS_POOL_SZ 1024
            pScale[1] = float(nShadowMapSize) / nShadowsPoolSizeY;
            pOffset[0] = float(packX[nSide]) / nShadowsPoolSizeX;
            pOffset[1] = float(packY[nSide]) / nShadowsPoolSizeY;
        }
        else
        {
            pOffset[0] = 1.0f / 3.0f * (nSide % 3);
            pOffset[1] = 1.0f / 2.0f * (nSide / 3);
            pScale[0] = 1.0f / 3.0f;
            pScale[1] = 1.0f / 2.0f;
        }
    }

    void RequestUpdate()
    {
        for (int i = 0; i < MAX_GPU_NUM; i++)
        {
            nInvalidatedFrustMask[i] = 0x3F;
        }
    }

    bool isUpdateRequested(int nMaskNum)
    {
        assert(nMaskNum >= 0 && nMaskNum < MAX_GPU_NUM);
        /*if (nMaskNum==-1) //request from 3dengine
        {
          for (int i=0; i<MAX_GPU_NUM; i++)
          {
            if(nInvalidatedFrustMask[i]>0)
              return true;
          }
          return false;
        }*/

        return (nInvalidatedFrustMask[nMaskNum] > 0);
    }

    bool IsCached() const
    {
        return m_eFrustumType == e_GsmCached || m_eFrustumType == e_HeightMapAO;
    }

    ILINE bool IntersectAABB(const AABB& bbox, bool* pAllIn) const
    {
        if (bOmniDirectionalShadow)
        {
            return bbox.IsOverlapSphereBounds(vLightSrcRelPos + vProjTranslation, fFarDist);
        }
        else
        {
            bool bDummy = false;
            if (bBlendFrustum)
            {
                if (FrustumPlanes[1].IsAABBVisible_EH(bbox, pAllIn) > 0)
                {
                    return true;
                }
            }

            return FrustumPlanes[0].IsAABBVisible_EH(bbox, bBlendFrustum ? &bDummy : pAllIn) > 0;
        }
    }

    ILINE bool IntersectSphere(const Sphere& sp, bool* pAllIn)
    {
        if (bOmniDirectionalShadow)
        {
            return Distance::Point_PointSq(sp.center, vLightSrcRelPos + vProjTranslation) < sqr(fFarDist + sp.radius);
        }
        else
        {
            uint8 res = 0;
            if (bBlendFrustum)
            {
                res = FrustumPlanes[1].IsSphereVisible_FH(sp);
                * pAllIn = (res == CULL_INCLUSION);
                if (res != CULL_EXCLUSION)
                {
                    return true;
                }
            }
            res = FrustumPlanes[0].IsSphereVisible_FH(sp);
            * pAllIn = bBlendFrustum ? false : (res == CULL_INCLUSION);
            return res != CULL_EXCLUSION;
        }
    }

    void UnProject(float sx, float sy, float sz, float* px, float* py, float* pz, IRenderer* pRend)
    {
        const int shadowViewport[4] = {0, 0, 1, 1};

        Matrix44A mIden;
        mIden.SetIdentity();

        //FIX remove float arrays
        pRend->UnProject(sx, sy, sz,
            px, py, pz,
            (float*)&mLightViewMatrix,
            (float*)&mIden,
            shadowViewport);
    }

    Vec3& UnProjectVertex3d(int sx, int sy, int sz, Vec3& vert, IRenderer* pRend)
    {
        float px;
        float py;
        float pz;
        UnProject((float)sx, (float)sy, (float)sz, &px, &py, &pz, pRend);
        vert.x = (float)px;
        vert.y = (float)py;
        vert.z = (float)pz;

        //      pRend->DrawBall(vert,10);

        return vert;
    }

    void UpdateOmniFrustums()
    {
        const float sCubeVector[6][7] =
        {
            {1, 0, 0,  0, 0, 1, -90}, //posx
            {-1, 0, 0, 0, 0, 1,  90}, //negx
            {0, 1, 0,  0, 0, -1, 0},  //posy
            {0, -1, 0, 0, 0, 1,  0},  //negy
            {0, 0, 1,  0, 1, 0,  0},  //posz
            {0, 0, -1, 0, 1, 0,  0},  //negz
        };

        const Vec3 vPos = vLightSrcRelPos + vProjTranslation;
        for (int nS = 0; nS < OMNI_SIDES_NUM; ++nS)
        {
            const Vec3 vForward   = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
            const Vec3 vUp        = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
            const Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[nS][6]));
            const float fov = bUnwrapedOmniDirectional ? (float)DEG2RAD_R(g_fOmniShadowFov) : (float)DEG2RAD_R(90.0f);

            FrustumPlanes[nS].SetMatrix(Matrix34(matRot, vPos));
            FrustumPlanes[nS].SetFrustum(nTexSize, nTexSize, fov, fNearDist, fFarDist);
        }
    }

    void DrawFrustum(IRenderer* pRend, int nFrames = 1)
    {
        if (abs(nUpdateFrameId - pRend->GetFrameID()) > nFrames)
        {
            return;
        }

        //if(!arrLightViewMatrix[0] && !arrLightViewMatrix[5] && !arrLightViewMatrix[10])
        //return;

        IRenderAuxGeom* pRendAux = pRend->GetIRenderAuxGeom();
        const ColorF cCascadeColors[] = { Col_Red, Col_Green, Col_Blue, Col_Yellow, Col_Magenta, Col_Cyan, Col_Black, Col_White };
        const uint nColorCount = sizeof(cCascadeColors) / sizeof(cCascadeColors[0]);

        Vec3 vert1, vert2;
        ColorB c0 = cCascadeColors[nShadowMapLod % nColorCount];
        {
            pRendAux->DrawLine(
                UnProjectVertex3d(0, 0, 0, vert1, pRend), c0,
                UnProjectVertex3d(0, 0, 1, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(1, 0, 0, vert1, pRend), c0,
                UnProjectVertex3d(1, 0, 1, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(1, 1, 0, vert1, pRend), c0,
                UnProjectVertex3d(1, 1, 1, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(0, 1, 0, vert1, pRend), c0,
                UnProjectVertex3d(0, 1, 1, vert2, pRend), c0);
        }

        for (int i = 0; i <= 1; i++)
        {
            pRendAux->DrawLine(
                UnProjectVertex3d(0, 0, i, vert1, pRend), c0,
                UnProjectVertex3d(1, 0, i, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(1, 0, i, vert1, pRend), c0,
                UnProjectVertex3d(1, 1, i, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(1, 1, i, vert1, pRend), c0,
                UnProjectVertex3d(0, 1, i, vert2, pRend), c0);

            pRendAux->DrawLine(
                UnProjectVertex3d(0, 1, i, vert1, pRend), c0,
                UnProjectVertex3d(0, 0, i, vert2, pRend), c0);
        }
    }

    void ResetCasterLists()
    {
        m_castersList.Clear();
        m_jobExecutedCastersList.Clear();
    }

    void GetMemoryUsage(ICrySizer* pSizer) const;
};

struct ShadowFrustumMGPUCache
    : public ISyncMainWithRenderListener
{
    StaticArray<ShadowMapFrustum*, MAX_GSM_LODS_NUM> m_pStaticShadowMapFrustums;
    ShadowMapFrustum* m_pHeightMapAOFrustum;

    uint32 nUpdateMaskMT;
    uint32 nUpdateMaskRT;

    ShadowFrustumMGPUCache()
        : nUpdateMaskMT(0)
        , nUpdateMaskRT(0)
    {
        m_pHeightMapAOFrustum = NULL;
        m_pStaticShadowMapFrustums.fill(NULL);
    };

    void Init()
    {
        m_pHeightMapAOFrustum = new ShadowMapFrustum;
        m_pHeightMapAOFrustum->pShadowCacheData = new ShadowMapFrustum::ShadowCacheData;

        for (int i = 0; i < m_pStaticShadowMapFrustums.size(); ++i)
        {
            m_pStaticShadowMapFrustums[i] = new ShadowMapFrustum;
            m_pStaticShadowMapFrustums[i]->pShadowCacheData = new ShadowMapFrustum::ShadowCacheData;
        }

        nUpdateMaskMT = nUpdateMaskRT = 0;
    }

    void Release()
    {
        SAFE_DELETE(m_pHeightMapAOFrustum);
        for (int i = 0; i < m_pStaticShadowMapFrustums.size(); ++i)
        {
            SAFE_DELETE(m_pStaticShadowMapFrustums[i]);
        }
    }

    void DeleteFromCache(IShadowCaster* pCaster)
    {
        for (int i = 0; i < m_pStaticShadowMapFrustums.size(); ++i)
        {
            ShadowMapFrustum* pFr = m_pStaticShadowMapFrustums[i];
            if (pFr)
            {
                pFr->m_castersList.Delete(pCaster);
                pFr->m_jobExecutedCastersList.Delete(pCaster);
            }
        }

        if (m_pHeightMapAOFrustum)
        {
            m_pHeightMapAOFrustum->m_castersList.Delete(pCaster);
            m_pHeightMapAOFrustum->m_jobExecutedCastersList.Delete(pCaster);
        }
    }

    virtual void SyncMainWithRender()
    {
        /** What we need here is the renderer telling the main thread to update the shadow frustum cache when all GPUs are done
        * with the current frustum.
        *
        * So in case the main thread has done a full update (nUpdateMaskMT has bits for all GPUs set) we need to copy
        * the update mask to the renderer. Note that we reset the main thread update mask in the same spot to avoid doing it in
        * the next frame again.
        *
        * Otherwise just copy the renderer's progress back to the main thread. The main thread will automatically do a full update
        * when nUpdateMaskMT reaches 0                                                                                                                                                                                  */
        const int nFullUpdateMask = (1 << gEnv->pRenderer->GetActiveGPUCount()) - 1;
        if (nUpdateMaskMT == nFullUpdateMask)
        {
            nUpdateMaskRT = nUpdateMaskMT;
            nUpdateMaskMT = 0xFFFFFFFF;
        }
        else
        {
            nUpdateMaskMT = nUpdateMaskRT;
        }
    }
};

#endif
