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

#include "Cry3DEngine_precompiled.h"
#include "ShadowCache.h"
#include "LightEntity.h"
#include "VisAreas.h"

const float ShadowCache::AO_FRUSTUM_SLOPE_BIAS = 0.5f;

void ShadowCache::InitShadowFrustum(ShadowMapFrustum*& pFr, int nLod, int nFirstStaticLod, float fDistFromViewDynamicLod, float fRadiusDynamicLod, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;
    assert(nLod >= 0);

    // If we only allow updates via script, then early out here.
    // When script triggers an update, m_nUpdateStrategy will be set to ShadowMapFrustum::ShadowCacheData::eFullUpdate for a single frame to process a full cached shadow update
    if ( m_nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eManualUpdate )
    {
        return;
    }

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // ShadowCache not supported in render scene to texture passes yet.
    // It is very common to jump large distances.
    if (passInfo.IsRenderSceneToTexturePass())
    {
        return;
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    if (!pFr)
    {
        pFr = new ShadowMapFrustum;
    }

    if (!pFr->pShadowCacheData)
    {
        pFr->pShadowCacheData = new ShadowMapFrustum::ShadowCacheData;
    }
    
    // Check if we both allow updates if the user moves too close to the border of the shadow map and if we have come too close to said border
    ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy = m_nUpdateStrategy;
    bool allowDistanceBasedUpdates = nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eManualOrDistanceUpdate || nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate;
    if (allowDistanceBasedUpdates && Get3DEngine()->m_CachedShadowsBounds.IsReset())
    {
        // Distance from the camera to the center of the last time this cached static shadow map was updated
        const float distanceBetweenCenters = (passInfo.GetCamera().GetPosition() - pFr->aabbCasters.GetCenter()).GetLength();

        // Note: These values are calculated based on e_gsmRange, e_gsmRangeStep and your camera properties.  They will not change unless those change.
        const float dynamicCascadeMeasurements = fDistFromViewDynamicLod + fRadiusDynamicLod;

        // Check if our dynamic shadow frustum lies outside of the cached shadow frustum.  If so, force an update.
        if ((distanceBetweenCenters + dynamicCascadeMeasurements) > pFr->aabbCasters.GetSize().x / 2.0f)
        {
            nUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate;

            if (!gEnv->IsEditing())
            {
                // If our dynamic cascade always registers as larger than the update threshold for the cached frustum, regardless of how far the camera has moved, this is a serious error.
                // This needs fixed either via CVars or a flow graph node.
                if (dynamicCascadeMeasurements > (pFr->aabbCasters.GetSize().x / 2.0f))
                {
                    // To prevent per-frame spam, but still output often enough that this message will get noticed and hopefully fixed.
                    // Not fixing this issue means users are potentially rendering thousands of extra objects into shadow maps per frame.
                    static unsigned int s_lastOutput = 1000;
                    if ((++s_lastOutput) > 1000)
                    {
                        s_lastOutput = 0;
                        CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_ERROR, "Cached shadowmap %d is forced to update even though the camera has not moved or only slightly moved."                                                                                     \
                            "\tIf you see this output very often, and you are not using their default values, you may need to either increase the shadow cache resolution (r_ShadowsCacheResolutions) or decrease the global shadow map resolution (e_ShadowsMaxTexRes)." \
                            "\tOtherwise, add a flow graph node (Environment:RecomputeStaticShadows) that updates the cached shadowmaps for this region.",  nLod - nFirstStaticLod);
                    }
                }
                else
                {
                    // The camera most likely moved a far distance away from where the last cached shadowmap was rendered;
                    // however, the user did not add a flowgraph node to recompute the cached shadowmaps in this region
                    CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Update required for cached shadow map %d." \
                        "\tConsider setting up manual bounds for cached shadows in this region via flow graph (Environment:RecomputeStaticShadows) if this happens too often", nLod - nFirstStaticLod);
                }
            }
        }
    }

    // If we allow only manual or distance-based updates, then early out here if nUpdateStrategy was not set to ShadowMapFrustum::ShadowCacheData::eFullUpdate after the distance checks above.
    if ( nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eManualOrDistanceUpdate )
    {
        return;
    }

    AABB projectionBoundsLS(AABB::RESET);
    const int nTexRes = GetRenderer()->GetCachedShadowsResolution()[nLod - nFirstStaticLod];

    // non incremental update: set new bounding box and estimate near/far planes
    if (nUpdateStrategy != ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate)
    {
        Matrix34 matView = Matrix34(GetViewMatrix(passInfo).GetTransposed());

        if (!Get3DEngine()->m_CachedShadowsBounds.IsReset())
        {
            float fBoxScale = powf(Get3DEngine()->m_fCachedShadowsCascadeScale, (float)nLod - nFirstStaticLod);
            Vec3 fBoxScaleXY(max(1.f, fBoxScale));
            fBoxScaleXY.z = 1.f;

            Vec3 vExt = Get3DEngine()->m_CachedShadowsBounds.GetSize().CompMul(fBoxScaleXY * 0.5f);
            Vec3 vCenter = Get3DEngine()->m_CachedShadowsBounds.GetCenter();

            pFr->aabbCasters = AABB(vCenter - vExt, vCenter + vExt);
            projectionBoundsLS = AABB::CreateTransformedAABB(matView, pFr->aabbCasters);
        }
        else
        {
            const float fDesiredPixelDensity =  fRadiusDynamicLod / GetCVars()->e_ShadowsMaxTexRes;
            GetCasterBox(pFr->aabbCasters, projectionBoundsLS, fDesiredPixelDensity * nTexRes, matView, passInfo);
        }
    }

    // finally init frustum
    InitCachedFrustum(pFr, nUpdateStrategy, nLod, nTexRes, m_pLightEntity->m_light.m_Origin, projectionBoundsLS, passInfo);
    pFr->m_eFrustumType = ShadowMapFrustum::e_GsmCached;
    pFr->bBlendFrustum = GetCVars()->e_ShadowsBlendCascades > 0;
    pFr->fBlendVal = pFr->bBlendFrustum ? GetCVars()->e_ShadowsBlendCascadesVal : 1.0f;

    // frustum debug
    {
        const ColorF cascadeColors[] = { Col_Red, Col_Green, Col_Blue, Col_Yellow, Col_Magenta, Col_Cyan };
        const uint colorCount = sizeof(cascadeColors) / sizeof(cascadeColors[0]);

        if (GetCVars()->e_ShadowsCacheUpdate > 2)
        {
            Get3DEngine()->DrawBBox(pFr->aabbCasters, cascadeColors[pFr->nShadowMapLod % colorCount]);
        }

        if (GetCVars()->e_ShadowsFrustums > 0)
        {
            pFr->DrawFrustum(GetRenderer(), std::numeric_limits<int>::max());
        }
    }
}

void ShadowCache::InitCachedFrustum(ShadowMapFrustum*& pFr, ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy, int nLod, int nTexSize, const Vec3& vLightPos, const AABB& projectionBoundsLS, const SRenderingPassInfo& passInfo)
{
    pFr->ResetCasterLists();
    pFr->nTexSize = nTexSize;

    if (nUpdateStrategy != ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate)
    {
        pFr->pShadowCacheData->Reset();
        pFr->nShadowGenMask = 1;

        assert(m_pLightEntity->m_light.m_pOwner);
        pFr->pLightOwner = m_pLightEntity->m_light.m_pOwner;
        pFr->m_Flags = m_pLightEntity->m_light.m_Flags;
        pFr->nUpdateFrameId = passInfo.GetFrameID();
        pFr->nShadowMapLod = nLod;
        pFr->vProjTranslation = pFr->aabbCasters.GetCenter();
        pFr->vLightSrcRelPos = vLightPos - pFr->aabbCasters.GetCenter();
        pFr->fNearDist = -projectionBoundsLS.max.z;
        pFr->fFarDist  = -projectionBoundsLS.min.z;
        pFr->fFOV = (float)RAD2DEG(atan_tpl(0.5 * projectionBoundsLS.GetSize().y / pFr->fNearDist)) * 2.f;
        pFr->fProjRatio = projectionBoundsLS.GetSize().x / projectionBoundsLS.GetSize().y;
        pFr->fRadius = m_pLightEntity->m_light.m_fRadius;
        pFr->fFrustrumSize =  1.0f / (Get3DEngine()->m_fGsmRange * pFr->aabbCasters.GetRadius() * 2.0f);

        const float arrWidthS[] = {1.94f, 1.0f, 0.8f, 0.5f, 0.3f, 0.3f, 0.3f, 0.3f};
        pFr->fWidthS = pFr->fWidthT =  arrWidthS[nLod];
        pFr->fBlurS = pFr->fBlurT = 0.0f;
    }

    const int maxNodesPerFrame = (nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eFullUpdate)
        ? std::numeric_limits<int>::max()
        : MAX_RENDERNODES_PER_FRAME* GetRenderer()->GetActiveGPUCount();

    const bool bExcludeDynamicDistanceShadows = GetCVars()->e_DynamicDistanceShadows != 0;
    m_pObjManager->MakeStaticShadowCastersList(((CLightEntity*)m_pLightEntity->m_light.m_pOwner)->m_pNotCaster, pFr,
        bExcludeDynamicDistanceShadows ? ERF_DYNAMIC_DISTANCESHADOWS : 0, maxNodesPerFrame, passInfo);

    pFr->pShadowCacheData->mProcessedCasters.insert(pFr->m_castersList.begin(), pFr->m_castersList.end());
    pFr->pShadowCacheData->mProcessedCasters.insert(pFr->m_jobExecutedCastersList.begin(), pFr->m_jobExecutedCastersList.end());
    pFr->RequestUpdate();
    pFr->bIncrementalUpdate = nUpdateStrategy == ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate && !pFr->pShadowCacheData->mProcessedCasters.empty();
}

void ShadowCache::InitHeightMapAOFrustum(ShadowMapFrustum*& pFr, int nLod, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;
    assert(nLod >= 0);

    if (!pFr)
    {
        pFr = new ShadowMapFrustum;
    }

    if (!pFr->pShadowCacheData)
    {
        pFr->pShadowCacheData = new ShadowMapFrustum::ShadowCacheData;
    }

    static ICVar* pHeightMapAORes   = gEnv->pConsole->GetCVar("r_HeightMapAOResolution");
    static ICVar* pHeightMapAORange = gEnv->pConsole->GetCVar("r_HeightMapAORange");

    ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy = m_nUpdateStrategy;

    // check if we have come too close to the border of the map
    const float fDistFromCenter = (passInfo.GetCamera().GetPosition() - pFr->aabbCasters.GetCenter()).GetLength() + pHeightMapAORange->GetFVal() * 0.25f;
    if (fDistFromCenter > pFr->aabbCasters.GetSize().x / 2.0f)
    {
        nUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate;

        if (!gEnv->IsEditing())
        {
            CryLog("Update required for height map AO.");
            CryLog("\tConsider increasing height map AO range (r_HeightMapAORange) if this happens too often");
        }
    }

    AABB projectionBoundsLS(AABB::RESET);

    // non incremental update: set new bounding box and estimate near/far planes
    if (nUpdateStrategy != ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate)
    {
        // Top down view
        Matrix34 topDownView(IDENTITY);
        topDownView.m03 = -passInfo.GetCamera().GetPosition().x;
        topDownView.m13 = -passInfo.GetCamera().GetPosition().y;
        topDownView.m23 = -passInfo.GetCamera().GetPosition().z - m_pLightEntity->m_light.m_Origin.GetLength();

        GetCasterBox(pFr->aabbCasters, projectionBoundsLS, pHeightMapAORange->GetFVal() / 2.0f, topDownView, passInfo);

        // snap to texels
        const float fSnap = pHeightMapAORange->GetFVal() / pHeightMapAORes->GetFVal();
        pFr->aabbCasters.min.x = fSnap * int(pFr->aabbCasters.min.x / fSnap);
        pFr->aabbCasters.min.y = fSnap * int(pFr->aabbCasters.min.y / fSnap);
        pFr->aabbCasters.min.z = fSnap * int(pFr->aabbCasters.min.z / fSnap);

        pFr->aabbCasters.max.x = pFr->aabbCasters.min.x + pHeightMapAORange->GetFVal();
        pFr->aabbCasters.max.y = pFr->aabbCasters.min.y + pHeightMapAORange->GetFVal();
        pFr->aabbCasters.max.z = fSnap * int(pFr->aabbCasters.max.z / fSnap);

        pFr->fDepthSlopeBias = AO_FRUSTUM_SLOPE_BIAS;
        pFr->fDepthConstBias = 0;

        pFr->mLightViewMatrix.SetIdentity();
        pFr->mLightViewMatrix.m30 = -pFr->aabbCasters.GetCenter().x;
        pFr->mLightViewMatrix.m31 = -pFr->aabbCasters.GetCenter().y;
        pFr->mLightViewMatrix.m32 = -pFr->aabbCasters.GetCenter().z - m_pLightEntity->m_light.m_Origin.GetLength();

        mathMatrixOrtho(&pFr->mLightProjMatrix, projectionBoundsLS.GetSize().x, projectionBoundsLS.GetSize().y, -projectionBoundsLS.max.z, -projectionBoundsLS.min.z);
    }

    const Vec3 lightPos = pFr->aabbCasters.GetCenter() + Vec3(0, 0, 1) * m_pLightEntity->m_light.m_Origin.GetLength();

    // finally init frustum
    const int nTexRes = (int)clamp_tpl(pHeightMapAORes->GetFVal(), 0.f, 16384.f);
    InitCachedFrustum(pFr, nUpdateStrategy, nLod, nTexRes, lightPos, projectionBoundsLS, passInfo);
    pFr->m_eFrustumType = ShadowMapFrustum::e_HeightMapAO;
}

void ShadowCache::GetCasterBox(AABB& BBoxWS, AABB& BBoxLS, float fRadius, const Matrix34& matView, const SRenderingPassInfo& passInfo)
{
    AABB projectionBoundsLS;

    BBoxWS = AABB(passInfo.GetCamera().GetPosition(), fRadius);
    BBoxLS = AABB(matView.TransformPoint(passInfo.GetCamera().GetPosition()), fRadius);

    // try to get tighter near/far plane from casters
    AABB casterBoxLS(AABB::RESET);
    if (Get3DEngine()->IsObjectTreeReady())
    {
        casterBoxLS.Add(Get3DEngine()->GetObjectTree()->GetShadowCastersBox(&BBoxWS, &matView));
    }

    if (CVisAreaManager* pVisAreaManager = GetVisAreaManager())
    {
        for (int i = 0; i < pVisAreaManager->m_lstVisAreas.Count(); ++i)
        {
            if (pVisAreaManager->m_lstVisAreas[i] && pVisAreaManager->m_lstVisAreas[i]->m_pObjectsTree)
            {
                casterBoxLS.Add(pVisAreaManager->m_lstVisAreas[i]->m_pObjectsTree->GetShadowCastersBox(&BBoxWS, &matView));
            }
        }

        for (int i = 0; i < pVisAreaManager->m_lstPortals.Count(); ++i)
        {
            if (pVisAreaManager->m_lstPortals[i] && pVisAreaManager->m_lstPortals[i]->m_pObjectsTree)
            {
                casterBoxLS.Add(pVisAreaManager->m_lstPortals[i]->m_pObjectsTree->GetShadowCastersBox(&BBoxWS, &matView));
            }
        }
    }

    if (!casterBoxLS.IsReset() && casterBoxLS.GetSize().z < 2 * fRadius)
    {
        float fDepthRange = 2.0f * max(Get3DEngine()->m_fSunClipPlaneRange, casterBoxLS.GetSize().z);
        BBoxLS.max.z = casterBoxLS.max.z + 0.5f; // slight offset here to counter edge case where polygons are projection plane aligned and would come to lie directly on the near plane
        BBoxLS.min.z = casterBoxLS.max.z - fDepthRange;
    }
}

Matrix44 ShadowCache::GetViewMatrix(const SRenderingPassInfo& passInfo)
{
    const Vec3 zAxis(0.f, 0.f, 1.f);
    const Vec3 yAxis(0.f, 1.f, 0.f);

    Vec3 At = passInfo.GetCamera().GetPosition();
    Vec3 Eye = m_pLightEntity->m_light.m_Origin;
    Vec3 Up = fabsf((Eye - At).GetNormalized().Dot(zAxis)) > 0.9995f ? yAxis : zAxis;

    Matrix44 result;
    mathMatrixLookAt(&result, Eye, At, Up);

    return result;
}

ILINE uint64 ShadowCache::HashValue(uint64 value)
{
    uint64 hash = value * kHashMul;
    hash ^= (hash >> 47);
    hash *= kHashMul;
    return hash;
}
