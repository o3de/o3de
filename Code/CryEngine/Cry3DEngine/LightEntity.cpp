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
#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "LightEntity.h"
#include "CullBuffer.h"
#include "IShader.h"
#include "ClipVolume.h"
#include "ClipVolumeManager.h"
#include "ShadowCache.h"

#include <MathConversion.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

PodArray<SPlaneObject> CLightEntity::s_lstTmpCastersHull;

#define MIN_SHADOW_RES_OMNI_LIGHT 64
#define MIN_SHADOW_RES_PROJ_LIGHT 128

void CLightEntity::StaticReset()
{
    stl::free_container(s_lstTmpCastersHull);
}

void CLightEntity::InitEntityShadowMapInfoStructure()
{
    // Init ShadowMapInfo structure
    if (!m_pShadowMapInfo)
    {
        m_pShadowMapInfo = new ShadowMapInfo(); // leak
    }
}

CLightEntity::CLightEntity()
    : m_VoxelGIMode{VM_None}
{
    m_layerId = ~0;

    m_bShadowCaster = false;
    m_pShadowMapInfo = NULL;
    m_pNotCaster = NULL;

    memset(&m_Matrix, 0, sizeof(m_Matrix));

    m_pStatObj = NULL;
    GetInstCount(GetRenderNodeType())++;
}

CLightEntity::~CLightEntity()
{
    SAFE_RELEASE(m_light.m_Shader.m_pShader);

    Get3DEngine()->FreeRenderNodeState(this);

    ((C3DEngine*)Get3DEngine())->RemoveEntityLightSources(this);

    // delete shadow frustums
    if (m_pShadowMapInfo)
    {
        for (int nLod = 0; nLod < MAX_GSM_LODS_NUM; nLod++)
        {
            //TODO: after porting the sorting to jobs, add a sync point here to prevent deleting a ShadowFrustum which could still be used by a job
            SAFE_DELETE(m_pShadowMapInfo->pGSM[nLod]);
        }
    }
    SAFE_DELETE(m_pShadowMapInfo);

    GetInstCount(GetRenderNodeType())--;

    SAFE_RELEASE(m_pStatObj);
}

const char* CLightEntity::GetName(void) const
{
    return m_Name.size() ? m_Name.c_str() : (m_light.m_sName ? m_light.m_sName : "LightEntity");
}

bool CLightEntity::IsLightAreasVisible()
{
    IVisArea* pArea = GetEntityVisArea();

    // test area vis
    if (!pArea || pArea->GetVisFrameId() == GetRenderer()->GetFrameID())
    {
        return true; // visible
    }
    if (m_light.m_Flags & DLF_THIS_AREA_ONLY)
    {
        return false;
    }

    // test neighbors
    IVisArea* Areas[64];
    int nCount = pArea->GetVisAreaConnections(Areas, 64);
    for (int i = 0; i < nCount; i++)
    {
        if (Areas[i]->GetVisFrameId() == GetRenderer()->GetFrameID())
        {
            return true; // visible
        }
    }
    return false; // not visible
}


//////////////////////////////////////////////////////////////////////////
int CLightEntity::GetSlotCount() const
{
    if (m_pStatObj)
    {
        return 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CLightEntity::SetMatrix(const Matrix34& mat)
{
    m_Matrix = mat;
    Vec3 wp = mat.GetTranslation();
    if (!(m_light.m_Flags & DLF_DEFERRED_CUBEMAPS))
    {
        float fRadius = m_light.m_fRadius;
        if (m_light.m_Flags & DLF_AREA_LIGHT) // Use max for area lights.
        {
            fRadius += max(m_light.m_fAreaWidth, m_light.m_fAreaHeight);
        }
        SetBBox(AABB(wp - Vec3(fRadius), wp + Vec3(fRadius)));
    }
    else
    {
        OBB obb(OBB::CreateOBBfromAABB(Matrix33(m_Matrix), AABB(-m_light.m_ProbeExtents, m_light.m_ProbeExtents)));
        SetBBox(AABB::CreateAABBfromOBB(wp, obb));
    }
    m_light.SetPosition(wp);
    
    // Updating light properties here can permanently set our m_bShadowCaster to false if e_shadows is 0 so preserve it.
    // really updating the matrix shouldn't change our shadow casting property and we should probably make a new function 
    // to update matrix related properties instead of calling SetLightProperties
    bool isShadowCaster = m_bShadowCaster;
    SetLightProperties(m_light);
    m_bShadowCaster = isShadowCaster;

    Get3DEngine()->RegisterEntity(this);

    if (!memcmp(&m_Matrix, &mat, sizeof(Matrix34)))
    {
        return;
    }

    //update shadow frustums
    if (m_pShadowMapInfo != NULL)
    {
        for (int i = 0; i < MAX_GSM_LODS_NUM && m_pShadowMapInfo->pGSM[i] != NULL; i++)
        {
            m_pShadowMapInfo->pGSM[i]->RequestUpdate();
        }
    }
}


void C3DEngine::UpdateSun(const SRenderingPassInfo& passInfo)
{
    if (GetCVars()->e_Sun)
    {
        if (!m_pSun)
        {
            m_pSun = (CLightEntity*)CreateLightSource();
        }

        CDLight DynLight;
        DynLight.SetPosition(passInfo.GetCamera().GetPosition() + GetSunDir());
        DynLight.m_fRadius  = 100000000;
        DynLight.SetLightColor(GetSunColor());
        DynLight.SetSpecularMult(GetGlobalParameter(E3DPARAM_SUN_SPECULAR_MULTIPLIER));
        DynLight.m_Flags |= DLF_DIRECTIONAL | DLF_SUN | DLF_THIS_AREA_ONLY | DLF_LM | DLF_SPECULAROCCLUSION |
            ((m_bSunShadows && passInfo.RenderShadows()) ? DLF_CASTSHADOW_MAPS : 0);
        DynLight.m_sName = "Sun";

        DynLight.m_nLightStyle = gEnv->p3DEngine->GetSunAnimIndex();
        DynLight.SetAnimSpeed(gEnv->p3DEngine->GetSunAnimSpeed());
        DynLight.m_nLightPhase = gEnv->p3DEngine->GetSunAnimPhase();

        m_pSun->SetLightProperties(DynLight);

        m_pSun->SetBBox(AABB(
                DynLight.m_Origin - Vec3(DynLight.m_fRadius, DynLight.m_fRadius, DynLight.m_fRadius),
                DynLight.m_Origin + Vec3(DynLight.m_fRadius, DynLight.m_fRadius, DynLight.m_fRadius)));

        m_pSun->SetRndFlags(ERF_OUTDOORONLY, true);

        CDLight* lightProps = &m_pSun->GetLightProperties(); //We want the address of the CDLight stored in the sun, not the address of DynLight
        GetRenderer()->EF_UpdateDLight(lightProps);

        //Update the sun's animated color with the color calculated in EF_UpdateDLight
        gEnv->p3DEngine->SetSunAnimColor(Vec3(lightProps->m_Color.r, lightProps->m_Color.g, lightProps->m_Color.b));

        RegisterEntity(m_pSun);
    }
    else if (m_pSun)
    {
        UnRegisterEntityAsJob(m_pSun);
        DeleteLightSource(m_pSun);
        m_pSun = nullptr;
    }
}

Vec3 CLightEntity::GSM_GetNextScreenEdge(float fPrevRadius, float fPrevDistanceFromView, const SRenderingPassInfo& passInfo)
{
    Vec3 vEdgeN = passInfo.GetCamera().GetEdgeN();
    Vec3 vEdgeF = passInfo.GetCamera().GetEdgeF();
    Vec3 vPrevSphereCenter(0.0f, fPrevDistanceFromView, 0.0f);

    float fDistToFrustEdge = (((vPrevSphereCenter - vEdgeN).Cross(vEdgeF - vEdgeN)).GetLength()) / ((vEdgeF - vEdgeN).GetLength());
    float fDistToPlaneOnEdgeSquared = fPrevRadius * fPrevRadius - fDistToFrustEdge * fDistToFrustEdge;
    float fDistToPlaneOnEdge = fDistToPlaneOnEdgeSquared > 0.0f ? sqrt_tpl(fDistToPlaneOnEdgeSquared) : 0.0f;

    Vec3 vEdgeDir = (vEdgeF - vEdgeN);
    vEdgeDir.SetLength(2.0f * fDistToPlaneOnEdge);
    return vEdgeN + vEdgeDir;
};

float CLightEntity::GSM_GetLODProjectionCenter(const Vec3& vEdgeScreen, float fRadius)
{
    float fScreenEdgeSq = vEdgeScreen.z * vEdgeScreen.z + vEdgeScreen.x * vEdgeScreen.x;
    float fRadiusSq = max(fRadius * fRadius, 2.0f * fScreenEdgeSq);
    float fDistanceFromNear = sqrt_tpl(fRadiusSq - fScreenEdgeSq);
    float fDistanceFromView = fDistanceFromNear + vEdgeScreen.y;
    return fDistanceFromView;
}


void CLightEntity::UpdateGSMLightSourceShadowFrustum(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    const int nMaxLodCount = min(GetCVars()->e_GsmLodsNum, MAX_GSM_LODS_NUM);
    int nDynamicLodCount = nMaxLodCount;
    int nCachedLodCount = 0;

    // check for shadow cache
    if (m_light.m_Flags & DLF_SUN)
    {
        int nFirstCachedLod = GetCVars()->e_ShadowsCache ? Get3DEngine()->m_nGsmCache - 1 : -1;
        if (nFirstCachedLod >= 0)
        {
            nDynamicLodCount = clamp_tpl(nFirstCachedLod, 0, nMaxLodCount);
            nCachedLodCount = nMaxLodCount - nDynamicLodCount;
        }
    }
    else
    {
        // If a light is not the SUN, then we don't take e_GsmLodsNum 
        // into account and decide we will have one lod.
        nDynamicLodCount = 1;
    }

    // update dynamic and static frustums
    float fDistFromView = 0;
    float fRadiusLastLod = 0;

    int nNextLod = 0;
    nNextLod  = UpdateGSMLightSourceDynamicShadowFrustum(nDynamicLodCount, nCachedLodCount, fDistFromView, fRadiusLastLod, nCachedLodCount == 0, passInfo);
    nNextLod += UpdateGSMLightSourceCachedShadowFrustum(nDynamicLodCount, nCachedLodCount, fDistFromView, fRadiusLastLod, passInfo);

    // free not used frustums
    for (int nLod = nNextLod; nLod < MAX_GSM_LODS_NUM; nLod++)
    {
        if (ShadowMapFrustum* pFr = m_pShadowMapInfo->pGSM[nLod])
        {
            pFr->ResetCasterLists();
            pFr->m_eFrustumType = ShadowMapFrustum::e_GsmDynamic;
            SAFE_DELETE(pFr->pShadowCacheData);
        }
    }
}

int CLightEntity::UpdateGSMLightSourceDynamicShadowFrustum(int nDynamicLodCount, int nDistanceLodCount, float& fDistanceFromViewNextDynamicLod, float& fGSMBoxSizeNextDynamicLod, bool bFadeLastCascade, const SRenderingPassInfo& passInfo)
{
    InitEntityShadowMapInfoStructure();

    float fGSMBoxSize = fGSMBoxSizeNextDynamicLod = (float)Get3DEngine()->m_fGsmRange;
    Vec3 vCameraDir = passInfo.GetCamera().GetMatrix().GetColumn(1).GetNormalized();
    float fDistToLight = passInfo.GetCamera().GetPosition().GetDistance(GetPos(true));

    PodArray<SPlaneObject>& lstCastersHull = s_lstTmpCastersHull;
    lstCastersHull.Clear();

    // prepare shadow frustums
    int nLod;

    //compute distance for first LOD
    Vec3 vEdgeScreen = passInfo.GetCamera().GetEdgeN();
    //clamp first frustum to DRAW_NEAREST_MIN near plane because weapon can be placed beyond camera near plane in world space
    vEdgeScreen.y = min(vEdgeScreen.y, DRAW_NEAREST_MIN);
    float fDistanceFromView = fDistanceFromViewNextDynamicLod = GSM_GetLODProjectionCenter(vEdgeScreen, Get3DEngine()->m_fGsmRange);

    for (nLod = 0; nLod < nDynamicLodCount + nDistanceLodCount; nLod++)
    {
        float fFOV = (m_light).m_fLightFrustumAngle * 2;
        bool bDoGSM = (fGSMBoxSize < m_light.m_fRadius * 0.01f && fGSMBoxSize < fDistToLight * 0.5f * (fFOV / 90.f) && fDistToLight < m_light.m_fRadius)
            && ((m_light.m_Flags & DLF_SUN) || Get3DEngine()->GetShadowsCascadeCount(&m_light) > 1);

        if (bDoGSM)
        {
            //Vec3 vSunDir = Get3DEngine()->GetSunDir().GetNormalized();  // todo: remove GetNormalized() once GetSunDir() returns the normalized value
            Vec3 vSunDir = Vec3(1.0f, 0.0f, 0.0f);
            Vec3 vCameraDirWithoutDepth = vCameraDir - vCameraDir.Dot(vSunDir) * vSunDir;

            Vec3 vFocusPos = passInfo.GetCamera().GetPosition() + vCameraDirWithoutDepth * fGSMBoxSize;
            SetBBox(AABB(vFocusPos - Vec3(fGSMBoxSize, fGSMBoxSize, fGSMBoxSize), vFocusPos + Vec3(fGSMBoxSize, fGSMBoxSize, fGSMBoxSize)));
        }
        else
        {
            float fRadius = m_light.m_fRadius;
            if (m_light.m_Flags & DLF_AREA_LIGHT) // Use max for area lights.
            {
                fRadius += max(m_light.m_fAreaWidth, m_light.m_fAreaHeight);
            }
            SetBBox(AABB(m_light.m_Origin - Vec3(fRadius), m_light.m_Origin + Vec3(fRadius)));
        }

        if (!m_pShadowMapInfo->pGSM[nLod])
        {
            m_pShadowMapInfo->pGSM[nLod] = new ShadowMapFrustum;
        }

        m_pShadowMapInfo->pGSM[nLod]->m_eFrustumType = nLod < nDynamicLodCount ? ShadowMapFrustum::e_GsmDynamic : ShadowMapFrustum::e_GsmDynamicDistance;
        m_pShadowMapInfo->pGSM[nLod]->bUseAdditiveBlending = false;
        m_pShadowMapInfo->pGSM[nLod]->fShadowFadingDist = (bFadeLastCascade && nLod == nDynamicLodCount - 1) ? 1.0f : 0.0f;

        if (!ProcessFrustum(nLod, bDoGSM ? fGSMBoxSize : 0, fDistanceFromView, lstCastersHull, passInfo))
        {
            nLod++;
            break;
        }

        //compute plane  for next GSM slice
        vEdgeScreen = GSM_GetNextScreenEdge(fGSMBoxSize, fDistanceFromView, passInfo);
        fGSMBoxSize *= Get3DEngine()->m_fGsmRangeStep;

        //compute distance from camera for next LOD
        fDistanceFromView = GSM_GetLODProjectionCenter(vEdgeScreen, fGSMBoxSize);

        if (nLod < nDynamicLodCount)
        {
            fDistanceFromViewNextDynamicLod = fDistanceFromView;
            fGSMBoxSizeNextDynamicLod = fGSMBoxSize;
        }
    }

    return nLod;
}

int CLightEntity::UpdateGSMLightSourceCachedShadowFrustum(int nFirstLod, int nLodCount, float& fDistFromViewDynamicLod, float fRadiusDynamicLod, const SRenderingPassInfo& passInfo)
{
    ShadowFrustumMGPUCache* pFrustumCache = GetRenderer()->GetShadowFrustumMGPUCache();
    assert(pFrustumCache);

    static ICVar* pHeightMapAOVar = GetConsole()->GetCVar("r_HeightMapAO");
    const int firstCachedFrustumIndex = nFirstLod + nLodCount;
    const bool bRestoreFromCache = GetRenderer()->GetActiveGPUCount() > 1 && pFrustumCache->nUpdateMaskMT != 0 &&  m_pShadowMapInfo->pGSM[firstCachedFrustumIndex];
    const bool bHeightMapAO = Get3DEngine()->m_bHeightMapAoEnabled && pHeightMapAOVar && pHeightMapAOVar->GetIVal() > 0 && (m_light.m_Flags & DLF_SUN);

    int nLod = 0;

    if (bRestoreFromCache)
    {
        for (nLod = 0; nLod < nLodCount; ++nLod)
        {
            assert(pFrustumCache->m_pStaticShadowMapFrustums[nLod] && m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod]);

            ShadowMapFrustum* pFr = m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod];
            *pFr = *pFrustumCache->m_pStaticShadowMapFrustums[nLod];
            pFr->bIsMGPUCopy = true;
        }

        if (bHeightMapAO)
        {
            assert(pFrustumCache->m_pHeightMapAOFrustum &&  m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod]);

            ShadowMapFrustum* pFr = m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod];
            *pFr = *pFrustumCache->m_pHeightMapAOFrustum;
            pFr->bIsMGPUCopy = true;

            ++nLod;
        }
    }
    else
    {
        ShadowMapFrustum::ShadowCacheData::eUpdateStrategy nUpdateStrategy =
            static_cast<ShadowMapFrustum::ShadowCacheData::eUpdateStrategy>(Get3DEngine()->m_nCachedShadowsUpdateStrategy);

        if (GetCVars()->e_ShadowsCacheUpdate)
        {
            nUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eFullUpdate;
        }

        ShadowCache shadowCache(this, nUpdateStrategy);

        for (nLod = 0; nLod < nLodCount; ++nLod)
        {
            ShadowMapFrustum*& pFr = m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod];
            shadowCache.InitShadowFrustum(pFr, nFirstLod + nLod, nFirstLod, fDistFromViewDynamicLod, fRadiusDynamicLod, passInfo);
            if (!pFr)
            {
                continue;
            }

            CalculateShadowBias(pFr, nFirstLod + nLod, fRadiusDynamicLod);
            pFr->bIsMGPUCopy = false;

            // update MGPU frustum cache
            if (GetRenderer()->GetActiveGPUCount() > 1 && pFrustumCache->m_pStaticShadowMapFrustums[nLod])
            {
                *pFrustumCache->m_pStaticShadowMapFrustums[nLod] = *pFr;
            }

            // update distance from view
            Vec3 vEdgeScreen = GSM_GetNextScreenEdge(fRadiusDynamicLod, fDistFromViewDynamicLod, passInfo);
            fRadiusDynamicLod *= Get3DEngine()->m_fGsmRangeStep;
            fDistFromViewDynamicLod = GSM_GetLODProjectionCenter(vEdgeScreen, fRadiusDynamicLod);
        }

        if (bHeightMapAO)
        {
            ShadowMapFrustum*& pFr = m_pShadowMapInfo->pGSM[firstCachedFrustumIndex + nLod];

            shadowCache.InitHeightMapAOFrustum(pFr, nFirstLod + nLod, passInfo);
            pFr->bIsMGPUCopy = false;
            if (GetRenderer()->GetActiveGPUCount() > 1 && pFrustumCache->m_pHeightMapAOFrustum)
            {
                *pFrustumCache->m_pHeightMapAOFrustum = *pFr;
            }

            ++nLod;
        }

        if (GetCVars()->e_ShadowsCacheUpdate == 1)
        {
            GetCVars()->e_ShadowsCacheUpdate = 0;
        }

        if (GetCVars()->e_ShadowsCacheRequireManualUpdate == 1)
        {
            Get3DEngine()->m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eManualUpdate;
        }
        else if (GetCVars()->e_ShadowsCacheRequireManualUpdate == 2)
        {
            Get3DEngine()->m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eManualOrDistanceUpdate;
        }
        else
        {
            Get3DEngine()->m_nCachedShadowsUpdateStrategy = ShadowMapFrustum::ShadowCacheData::eIncrementalUpdate;
        }

        int nActiveGPUCount = GetRenderer()->GetActiveGPUCount();
        pFrustumCache->nUpdateMaskMT = (1 << nActiveGPUCount) - 1;
    }

    return nLod;
}

bool CLightEntity::ProcessFrustum(int nLod, float fGSMBoxSize, float fDistanceFromView, PodArray<SPlaneObject>& lstCastersHull, const SRenderingPassInfo& passInfo)
{
    ShadowMapFrustum* pFr = m_pShadowMapInfo->pGSM[nLod];

    // make shadow map frustum for receiving (include all objects into frustum)
    assert(pFr);

    bool bDoGSM = fGSMBoxSize != 0;

    if (bDoGSM)// && (m_light.m_Flags & DLF_SUN || m_light.m_Flags & DLF_PROJECT))
    {
        InitShadowFrustum_SUN_Conserv(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, fGSMBoxSize, fDistanceFromView, nLod, passInfo);

        const uint32 renderNodeFlags = pFr->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance ? ERF_DYNAMIC_DISTANCESHADOWS : 0xFFFFFFFF;
        FillFrustumCastersList_SUN(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, renderNodeFlags, lstCastersHull, nLod, passInfo);
    }
    else if (m_light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT))
    {
        InitShadowFrustum_PROJECTOR(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, passInfo);
        FillFrustumCastersList_PROJECTOR(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, passInfo);
    }
    else
    {
        pFr->bOmniDirectionalShadow = true;
        InitShadowFrustum_OMNI(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, passInfo);
        FillFrustumCastersList_OMNI(pFr, SMC_EXTEND_FRUSTUM | SMC_SHADOW_FRUSTUM_TEST, passInfo);
    }

    CalculateShadowBias(pFr, nLod, fGSMBoxSize);

    if (GetCVars()->e_ShadowsFrustums && pFr && !pFr->m_castersList.IsEmpty())
    {
        pFr->DrawFrustum(GetRenderer(), (GetCVars()->e_ShadowsFrustums == 1) ? 1000 : 1);
    }


    return bDoGSM;
}

void CLightEntity::InitShadowFrustum_SUN_Conserv(ShadowMapFrustum* pFr, [[maybe_unused]] int dwAllowedTypes, float fGSMBoxSize, float fDistance, int nLod, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    assert(nLod >= 0 && nLod < MAX_GSM_LODS_NUM);

    //TOFIX: replace fGSMBoxSize  by fRadius
    float fRadius = fGSMBoxSize;
#ifdef FEATURE_SVO_GI
    if (gEnv->pConsole->GetCVar("e_svoTI_Active") && 
        gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal() && 
        nLod == 2)
    {
        fRadius += gEnv->pConsole->GetCVar("e_svoTI_ConeMaxLength")->GetFVal() * 0.5f;
    }
#endif

    Vec3 vViewDir = passInfo.GetCamera().GetViewdir();
    pFr->RequestUpdate();
    pFr->nShadowMapLod = nLod;
    pFr->vLightSrcRelPos = m_light.m_Origin - passInfo.GetCamera().GetPosition();
    pFr->fRadius = m_light.m_fRadius;
    assert(m_light.m_pOwner);
    pFr->pLightOwner = m_light.m_pOwner;
    pFr->m_Flags = m_light.m_Flags;
    pFr->bIncrementalUpdate = false;

    const AABB& box = GetBBox();
    const float fBoxRadius = max(0.00001f, box.GetRadius());

    //  float fDistToLightSrc = pFr->vLightSrcRelPos.GetLength();
    pFr->fFOV = (float)RAD2DEG(atan_tpl(fRadius / DISTANCE_TO_THE_SUN)) * 2.0f;
    if (pFr->fFOV > LIGHT_PROJECTOR_MAX_FOV)
    {
        pFr->fFOV = LIGHT_PROJECTOR_MAX_FOV;
    }
    pFr->fProjRatio = 1.0f;

    //Sampling parameters
    //Calculate proper projection of frustum to the terrain receiving area but not based on fBoxRadius

    float arrWidthS[] = {1.94f, 1.0f, 0.8f, 0.5f, 0.3f, 0.3f, 0.3f, 0.3f};

    pFr->fWidthS = arrWidthS[nLod];//fBoxRadiusMax/arrRangeS[nLod]; //fBoxRadius //*GetCVars()->e_ShadowsMaxTexRes *
    pFr->fWidthT = pFr->fWidthS;
    pFr->fBlurS = 0.0f;//arrBlurS[nLod];
    pFr->fBlurT = pFr->fBlurS;

    Vec3 vLightDir = pFr->vLightSrcRelPos.normalized();

    float fDist = pFr->vLightSrcRelPos.GetLength();

    //float fMaxFrustEdge = GSM_GetNextScreenEdge(fGSMBoxSize, fDistance).GetLength();
    Vec3 vEdgeN = passInfo.GetCamera().GetEdgeN();
    Vec3 vCamSpView(0.0f, fDistance, 0.0f);
    float fEdgeScale = (fDistance + fGSMBoxSize) / vEdgeN.y * vEdgeN.GetLength();
    float fMaxFrustEdge = ((vEdgeN.GetNormalized() * fEdgeScale) - vCamSpView).GetLength();
    fMaxFrustEdge *= 1.37f;

    {
        const float fDepthRange = 2.0f * max(Get3DEngine()->m_fSunClipPlaneRange, fMaxFrustEdge);
        const float fNearAdjust =  Lerp(fDepthRange - fMaxFrustEdge, fMaxFrustEdge, Get3DEngine()->m_fSunClipPlaneRangeShift);

        pFr->fNearDist = fDist - fNearAdjust;
        pFr->fFarDist = fDist + fDepthRange - fNearAdjust;
    }

    if (pFr->fFarDist > m_light.m_fRadius)
    {
        pFr->fFarDist = m_light.m_fRadius;
    }

    if (pFr->fNearDist < pFr->fFarDist * 0.005f)
    {
        pFr->fNearDist = pFr->fFarDist * 0.005f;
    }

    assert(pFr->fNearDist < pFr->fFarDist);

    pFr->nTexSize = GetCVars()->e_ShadowsMaxTexRes;

    pFr->vProjTranslation = passInfo.GetCamera().GetPosition() + fDistance * vViewDir;

    // local jitter amount depends on frustum size
    pFr->fFrustrumSize =  1.0f / (fGSMBoxSize * (float)Get3DEngine()->m_fGsmRange);
    pFr->nUpdateFrameId = passInfo.GetFrameID();
    pFr->bIncrementalUpdate = false;

    // setup the frustum main frustum plane before calculating frustum bounds and blending
    CCamera& FrustCam = pFr->FrustumPlanes[0] = CCamera();
    Matrix34A mat = Matrix33::CreateRotationVDir(-vLightDir);
    mat.SetTranslation(pFr->vLightSrcRelPos + pFr->vProjTranslation);
    FrustCam.SetMatrixNoUpdate(mat);
    FrustCam.SetFrustum(256, 256, pFr->fFOV * (gf_PI / 180.0f), pFr->fNearDist, pFr->fFarDist);

    GetGsmFrustumBounds(passInfo.GetCamera(), pFr);

    if (GetCVars()->e_ShadowsBlendCascades)
    {
        pFr->bBlendFrustum = true;
        pFr->fBlendVal = GetCVars()->e_ShadowsBlendCascadesVal;

        CCamera& BlendCam = pFr->FrustumPlanes[1] = pFr->FrustumPlanes[0];
        BlendCam.SetFrustum(256, 256, pFr->fBlendVal * pFr->fFOV * (gf_PI / 180.0f), pFr->fNearDist, pFr->fFarDist);
    }
    else
    {
        pFr->bBlendFrustum = false;
        pFr->fBlendVal = 1.0f;
    }
}

void CLightEntity::CalculateShadowBias(ShadowMapFrustum* pFr, int nLod, float fGSMBoxSize) const
{
    const float* arrDepthConstBias = Get3DEngine()->GetShadowsCascadesConstBias();
    const float* arrDepthSlopeBias = Get3DEngine()->GetShadowsCascadesSlopeBias();

    assert(nLod >= 0 && nLod < 8);

    if (m_light.m_Flags & DLF_SUN)
    {
        float fVladRatio = min(fGSMBoxSize / 2.f, 1.f);
        float fConstBiasRatio = arrDepthConstBias[nLod] * Get3DEngine()->m_fShadowsConstBias * fVladRatio;
        float fSlopeBiasRatio = arrDepthSlopeBias[nLod] * Get3DEngine()->m_fShadowsSlopeBias * fVladRatio;

        pFr->fDepthConstBias = fConstBiasRatio * (pFr->fFarDist - pFr->fNearDist) / (872727.27f * 2.0f) /** 0.000000001f;*/;
        pFr->fDepthTestBias  = fVladRatio * (pFr->fFarDist - pFr->fNearDist) * (fGSMBoxSize * 0.5f * 0.5f + 0.5f) * 0.0000005f;
        pFr->fDepthSlopeBias = fSlopeBiasRatio * (fGSMBoxSize / max(0.00001f, Get3DEngine()->m_fGsmRange)) * 0.1f;
    }
    else
    {
        pFr->fDepthConstBias  = m_light.m_fShadowBias * 0.000003f * pFr->fFarDist; //should be reverted to 0.0000001 after fixinf +X-frustum
        pFr->fDepthTestBias = 0.00028f * pFr->fFarDist;
        pFr->fDepthSlopeBias = m_light.m_fShadowSlopeBias * Get3DEngine()->m_fShadowsSlopeBias;
    }

    if (pFr->fDepthTestBias > 0.005f)
    {
        pFr->fDepthTestBias = 0.005f;
    }

    if (pFr->fNearDist < 1000.f) // if not sun
    {
        if (pFr->fDepthTestBias < 0.0005f)
        {
            pFr->fDepthTestBias = 0.0005f;
        }
    }
}

bool SegmentFrustumIntersection(const Vec3& P0, const Vec3& P1, const CCamera& frustum, Vec3* pvIntesectP0 = NULL, Vec3* pvIntesectP1 = NULL)
{
    if (P0.IsEquivalent(P1))
    {
        return frustum.IsPointVisible(P0);
    }

    //Actual Segment-Frustum intersection test
    float tE = 0.0f;
    float tL = 1.0f;
    Vec3 dS = P1 - P0;

    for (int i = 0; i < 6; i++)
    {
        const Plane* currPlane = frustum.GetFrustumPlane(i);

        Vec3 ni = currPlane->n;
        Vec3 Vi = ni * (-currPlane->d);

        float N = -(ni | Vec3(P0 - Vi));
        float D = ni | dS;

        if (D == 0) //segment is parallel to face
        {
            if (N < 0)
            {
                return false; //outside face
            }
            else
            {
                continue; //inside face
            }
        }

        float t = N / D;
        if (D < 0) //segment is entering face
        {
            tE = max(tE, t);
            if (tE > tL)
            {
                return false;
            }
        }
        else //segment is leaving face
        {
            tL = min(tL, t);
            if (tL < tE)
            {
                return false;
            }
        }
    }
    //calc intersection point if needed
    if (pvIntesectP0)
    {
        *pvIntesectP0 = P0 + tE * dS; // = P(tE) = point where S enters polygon
    }
    if (pvIntesectP1)
    {
        *pvIntesectP1 = P0 + tL * dS; // = P(tL) = point where S leaves polygon
    }
    //it's intersecting frustum
    return true;
}


bool CLightEntity::FrustumIntersection(const CCamera& viewFrustum, const CCamera& shadowFrustum)
{
    int i;

    Vec3 pvViewFrust[8];
    Vec3 pvShadowFrust[8];

    viewFrustum.GetFrustumVertices(pvViewFrust);
    shadowFrustum.GetFrustumVertices(pvShadowFrust);

    for (i = 0; i < 8; i++)
    {
        if (viewFrustum.IsPointVisible(pvShadowFrust[i]))
        {
            return true;
        }

        if (shadowFrustum.IsPointVisible(pvViewFrust[i]))
        {
            return true;
        }
    }

    for (i = 0; i < 4; i++)
    {
        //far face
        if (SegmentFrustumIntersection(pvShadowFrust[i], pvShadowFrust[(i + 1) % 4], viewFrustum))
        {
            return true;
        }
        //near face
        if (SegmentFrustumIntersection(pvShadowFrust[i + 4], pvShadowFrust[(i + 1) % 4 + 4], viewFrustum))
        {
            return true;
        }
        //other edges
        if (SegmentFrustumIntersection(pvShadowFrust[i], pvShadowFrust[i + 4], viewFrustum))
        {
            return true;
        }

        //vice-versa test
        //far face
        if (SegmentFrustumIntersection(pvViewFrust[i], pvViewFrust[(i + 1) % 4], shadowFrustum))
        {
            return true;
        }
        //near face
        if (SegmentFrustumIntersection(pvViewFrust[i + 4], pvViewFrust[(i + 1) % 4 + 4], shadowFrustum))
        {
            return true;
        }
        //other edges
        if (SegmentFrustumIntersection(pvViewFrust[i], pvViewFrust[i + 4], shadowFrustum))
        {
            return true;
        }
    }

    return false;
}

bool CLightEntity::GetGsmFrustumBounds(const CCamera& viewFrustum, ShadowMapFrustum* pShadowFrustum)
{
    int i;

    Vec3 pvViewFrust[8];
    Vec3 pvShadowFrust[8];

    CCamera& camShadowFrustum = pShadowFrustum->FrustumPlanes[0];

    Matrix34A mShadowView, mShadowProj;
    Matrix34A mShadowComposite = viewFrustum.GetMatrix().GetInverted();
    Matrix44A mCameraView = Matrix44A(viewFrustum.GetMatrix().GetInverted());
    viewFrustum.GetFrustumVertices(pvViewFrust);
    camShadowFrustum.GetFrustumVertices(pvShadowFrust);

    Vec3 vCamPosition = viewFrustum.GetPosition();
    Vec3 vIntersectP0(0, 0, 0), vIntersectP1(0, 0, 0);

    AABB viewAABB;
    viewAABB.Reset();

    bool bIntersected = false;
    f32 fDisatanceToMaxBound = 0;
    Vec3 vMaxBoundPoint(ZERO);

    for (i = 0; i < 4; i++)
    {
        //far face
        if (SegmentFrustumIntersection(pvShadowFrust[i], pvShadowFrust[(i + 1) % 4], viewFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }

        //near face
        if (SegmentFrustumIntersection(pvShadowFrust[i + 4], pvShadowFrust[(i + 1) % 4 + 4], viewFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }

        if (SegmentFrustumIntersection(pvShadowFrust[i], pvShadowFrust[i + 4], viewFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }

        if (SegmentFrustumIntersection(pvViewFrust[i], pvViewFrust[(i + 1) % 4], camShadowFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }

        if (SegmentFrustumIntersection(pvViewFrust[i + 4], pvViewFrust[(i + 1) % 4 + 4], camShadowFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }

        if (SegmentFrustumIntersection(pvViewFrust[i], pvViewFrust[i + 4], camShadowFrustum, &vIntersectP0, &vIntersectP1))
        {
            if (GetCVars()->e_GsmDepthBoundsDebug)
            {
                GetRenderer()->GetIRenderAuxGeom()->DrawLine(vIntersectP0, RGBA8(0xff, 0xff, 0x1f, 0xff), vIntersectP1, RGBA8(0xff, 0xff, 0x1f, 0xff), 2.0f);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP0, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
                GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vIntersectP1, RGBA8(0xff, 0xff, 0xff, 0xff), 10);
            }

            float fCurDistance = (vCamPosition - vIntersectP0).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP0;
                fDisatanceToMaxBound = fCurDistance;
            }

            fCurDistance = (vCamPosition - vIntersectP1).GetLength();
            if (fCurDistance > fDisatanceToMaxBound)
            {
                vMaxBoundPoint = vIntersectP1;
                fDisatanceToMaxBound = fCurDistance;
            }

            bIntersected = true;
        }
    }

    if (GetCVars()->e_GsmDepthBoundsDebug)
    {
        GetRenderer()->GetIRenderAuxGeom()->DrawPoint(vMaxBoundPoint, RGBA8(0xff, 0x00, 0x00, 0xff), 10);
    }

    return bIntersected;
}

void GetCubemapFrustum(ShadowMapFrustum* pFr, int nS, CCamera& shadowFrust)
{
    float sCubeVector[6][7] =
    {
        {1, 0, 0,  0, 0, 1, -90},//posx
        {-1, 0, 0, 0, 0, 1,  90},//negx
        {0, 1, 0,  0, 0, -1, 0},//posy
        {0, -1, 0, 0, 0, 1,  0},//negy
        {0, 0, 1,  0, 1, 0,  0},//posz
        {0, 0, -1, 0, 1, 0,  0},//negz
    };

    int nShadowTexSize = pFr->nTexSize;
    Vec3 vPos = pFr->vLightSrcRelPos + pFr->vProjTranslation;

    Vec3 vForward = Vec3(sCubeVector[nS][0], sCubeVector[nS][1], sCubeVector[nS][2]);
    Vec3 vUp      = Vec3(sCubeVector[nS][3], sCubeVector[nS][4], sCubeVector[nS][5]);
    Matrix33 matRot = Matrix33::CreateOrientation(vForward, vUp, DEG2RAD(sCubeVector[nS][6]));

    float fMinDist = pFr->fNearDist;
    float fMaxDist = pFr->fFarDist;
    shadowFrust.SetMatrix(Matrix34(matRot, vPos));
    shadowFrust.SetFrustum(nShadowTexSize, nShadowTexSize, 90.0f * gf_PI / 180.0f, fMinDist, fMaxDist);
}

void CLightEntity::CheckValidFrustums_OMNI(ShadowMapFrustum* pFr, const SRenderingPassInfo& passInfo)
{
    pFr->nOmniFrustumMask = 0;

    const CCamera& cameraFrust = passInfo.GetCamera();

    for (int nS = 0; nS < 6; nS++)
    {
        CCamera shadowFrust;
        GetCubemapFrustum(pFr, nS, shadowFrust);

        if (FrustumIntersection(cameraFrust, shadowFrust))
        {
            pFr->nOmniFrustumMask |= (1 << nS);
        }
    }
}

bool CLightEntity::CheckFrustumsIntersect(CLightEntity* lightEnt)
{
    Vec3 pvShadowFrust[8];
    bool bRes = false;

    ShadowMapFrustum* pFr1;
    ShadowMapFrustum* pFr2;

    pFr1 = this->GetShadowFrustum(0);
    pFr2 = lightEnt->GetShadowFrustum(0);

    if (!pFr1 || !pFr2)
    {
        return false;
    }

    int nFaces1, nFaces2;
    nFaces1 = (pFr1->bOmniDirectionalShadow ? 6 : 1);
    nFaces2 = (pFr2->bOmniDirectionalShadow ? 6 : 1);

    for (int nS1 = 0; nS1 < nFaces1; nS1++)
    {
        for (int nS2 = 0; nS2 < nFaces2; nS2++)
        {
            CCamera shadowFrust1 = pFr1->FrustumPlanes[nS1];
            CCamera shadowFrust2 = pFr2->FrustumPlanes[nS2];
            ;

            if (FrustumIntersection(shadowFrust1, shadowFrust2))
            {
                bRes = true;

                //debug frustums
                vtx_idx pnInd[8] = {
                    0, 4, 1, 5, 2, 6, 3, 7
                };
                //first frustum
                shadowFrust1.GetFrustumVertices(pvShadowFrust);
                GetRenderer()->GetIRenderAuxGeom()->DrawPolyline(pvShadowFrust, 4, true, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawPolyline(pvShadowFrust + 4, 4, true, RGBA8(0xff, 0xff, 0x1f, 0xff));

                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 2, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 4, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 6, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));

                //second frustum
                shadowFrust2.GetFrustumVertices(pvShadowFrust);
                GetRenderer()->GetIRenderAuxGeom()->DrawPolyline(pvShadowFrust, 4, true, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawPolyline(pvShadowFrust + 4, 4, true, RGBA8(0xff, 0xff, 0x1f, 0xff));

                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 2, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 4, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
                GetRenderer()->GetIRenderAuxGeom()->DrawLines(pvShadowFrust, 8, pnInd + 6, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
            }
        }
    }
    return bRes;
}

void CLightEntity::InitShadowFrustum_PROJECTOR(ShadowMapFrustum* pFr, [[maybe_unused]] int dwAllowedTypes, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    const int nFrameId = passInfo.GetMainFrameID();

    float fShadowUpdate = (float)(m_light.m_nShadowUpdateRatio * GetCVars()->e_ShadowsUpdateViewDistRatio);
    const float fShadowUpdateScale = (1 <<    DL_SHADOW_UPDATE_SHIFT) * (1 << DL_SHADOW_UPDATE_SHIFT);// e_ShadowsUpdateViewDistRatio is also fixed point, 256 == 1.0f

    // construct camera from projector
    Matrix34 entMat = ((ILightSource*)m_light.m_pOwner)->GetMatrix();

    Vec3 vProjDir = entMat.GetColumn(0).GetNormalizedSafe();

    pFr->nShadowMapLod = -1;    // not used

    // place center into middle of projector far plane
    pFr->vLightSrcRelPos = -vProjDir * m_light.m_fRadius;
    pFr->vProjTranslation = m_light.m_Origin - pFr->vLightSrcRelPos;
    if (pFr->fRadius != m_light.m_fRadius)
    {
        pFr->RequestUpdate();
    }
    pFr->bIncrementalUpdate = false;
    pFr->fRadius = m_light.m_fRadius;
    assert(m_light.m_pOwner && m_light.m_pOwner == this);
    pFr->pLightOwner = this;
    pFr->m_Flags = m_light.m_Flags;
    pFr->bBlendFrustum = false;

    //  float fDistToLightSrc = pFr->vLightSrcRelPos.GetLength();
    pFr->fFOV = CLAMP(m_light.m_fLightFrustumAngle * 2.f, 0.0001f, LIGHT_PROJECTOR_MAX_FOV);

    pFr->fNearDist = 0.01f;
    pFr->fFarDist = m_light.m_fRadius;

    // set texture size
    uint32 nTexSize = GetCVars()->e_ShadowsMaxTexRes;

    if (pFr->bOmniDirectionalShadow)
    {
        nTexSize = GetCVars()->e_ShadowsMaxTexRes / 2;
    }

    const CCamera& cCam = passInfo.GetCamera();

    float fLightToCameraDist = cCam.GetPosition().GetDistance(m_light.m_Origin);

    float fLightToCameraDistAdjusted = max(5.f, fLightToCameraDist - m_light.m_fRadius);
    while (nTexSize > (800.f / fLightToCameraDistAdjusted) * pFr->fRadius * m_light.m_Color.Luminance() * (pFr->fFOV / 90.f) && nTexSize > 256)
    {
        nTexSize /= 2;
    }

    float shadowUpdateDist = max(0.0f, fLightToCameraDist - m_light.m_fShadowUpdateMinRadius);

    float shadowUpdateRate = 255.0f;

    if (GetCVars()->e_ShadowsUpdateViewDistRatio)
    {
        shadowUpdateRate = min(255.f,  fShadowUpdateScale * shadowUpdateDist * passInfo.GetZoomFactor() / (fShadowUpdate));
    }

    pFr->nShadowPoolUpdateRate = (uint8) shadowUpdateRate;


    if (m_light.m_Flags & DLF_DEFERRED_LIGHT)
    {
        float fScaledRadius = m_light.m_fRadius * m_light.m_fShadowResolutionScale;
        //TD smooth distribution curve
        float fAreaZ0, fAreaZ1;
        if (fLightToCameraDist <= m_light.m_fRadius)
        {
            fAreaZ0 = cCam.GetNearPlane();
            fAreaZ1 = 2.0f * fScaledRadius;
        }
        else
        {
            fAreaZ0 = max(cCam.GetNearPlane(), fLightToCameraDist - fScaledRadius);
            fAreaZ1 = fLightToCameraDist + fScaledRadius;
        }


        float fCamFactor = log(cCam.GetFarPlane() / cCam.GetNearPlane());

        float fFadingBase = GetCVars()->e_ShadowsAdaptScale;
        float fSMZ0 = log(fAreaZ0 / cCam.GetNearPlane()) / log(fFadingBase);
        float fSMZ1 = log(fAreaZ1 / cCam.GetNearPlane()) / log(fFadingBase);
        fSMZ0 /= fCamFactor;
        fSMZ1 /= fCamFactor;

        float fCoverageScaleFactor = GetCVars()->e_ShadowsResScale;
        if (!(m_light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT)))
        {
            fCoverageScaleFactor /= 3.5f;
        }


        nTexSize = int((fSMZ1 - fSMZ0) * ((float)GetCVars()->e_ShadowsMaxTexRes * fCoverageScaleFactor /*/(7*cCam.GetFarPlane())*/));

        uint32 nPoolSize = GetCVars()->e_ShadowsPoolSize;
        uint32 nMaxTexRes = GetCVars()->e_ShadowsMaxTexRes;

        uint32 nMinRes, nMaxRes, nPhysicalMaxRes;

        if (m_light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT))
        {
            nMinRes = MIN_SHADOW_RES_PROJ_LIGHT;
            nMaxRes = nMaxTexRes;
            nPhysicalMaxRes = nPoolSize;
        }
        else
        {
            nMinRes = MIN_SHADOW_RES_OMNI_LIGHT;
            nMaxRes = nMaxTexRes >> 1;
            nPhysicalMaxRes = nPoolSize >> 2;
        }

        if (m_light.m_nShadowMinResolution)
        {
            nMinRes = nPoolSize >> (4 - m_light.m_nShadowMinResolution); // 4 possible percentages of pool size exposed in editor 100%, 50%, 25%, 12.5% (IEditorImpl.cpp)

            if (nMinRes > nMaxRes)           // if a very large min res is requested, go beyond normal limits, up to physical pool size. CINEMATICS ONLY PLEASE!
            {
                nMaxRes  = min(nPhysicalMaxRes, nMinRes);
            }
        }

        nTexSize = max(nMinRes, nTexSize);
        nTexSize = min(min(nMaxRes, nTexSize), nPhysicalMaxRes); // make sure we never go above pool size

        // force power of two
        nTexSize = 1 << IntegerLog2((uint32)nTexSize);
    }

    if (pFr->nTexSize != nTexSize)
    {
        pFr->nTexSize = nTexSize;
        pFr->RequestUpdate();
    }
    pFr->fFrustrumSize =  20.0f * nTexSize / 64.0f;
    pFr->nUpdateFrameId = nFrameId;
}

void CLightEntity::InitShadowFrustum_OMNI(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo)
{
    InitShadowFrustum_PROJECTOR(pFr, dwAllowedTypes, passInfo);
    CheckValidFrustums_OMNI(pFr, passInfo);
}

bool IsABBBVisibleInFrontOfPlane_FAST(const AABB& objBox, const SPlaneObject& clipPlane);

bool IsAABBInsideHull(const SPlaneObject* pHullPlanes, int nPlanesNum, const AABB& aabbBox)
{
    for (int i = 0; i < nPlanesNum; i++)
    {
        if (!IsABBBVisibleInFrontOfPlane_FAST(aabbBox, pHullPlanes[i]))
        {
            return false;
        }
    }

    return true;
}

bool IsSphereInsideHull(const SPlaneObject* pHullPlanes, int nPlanesNum, const Sphere& objSphere)
{
    for (int i = 0; i < nPlanesNum; i++)
    {
        if (-pHullPlanes[i].plane.DistFromPlane(objSphere.center) > objSphere.radius)
        {
            return false;
        }
    }
    return true;
}

int CLightEntity::MakeShadowCastersHullSun(PodArray<SPlaneObject>& lstCastersHull, const SRenderingPassInfo& passInfo)
{
    // Construct hull from camera vertices and light source position
    FUNCTION_PROFILER_3DENGINE;

    Vec3 vCamPos = passInfo.GetCamera().GetPosition();

    Vec3 arrFrustVerts[10];
    passInfo.GetCamera().GetFrustumVertices(arrFrustVerts);

    // 0 to 5 are the camera frustum vertices
    for (int v = 0; v < 4; v++)
    {
        arrFrustVerts[v] = vCamPos + (arrFrustVerts[v] - vCamPos).normalized() * GetObjManager()->GetGSMMaxDistance() * 1.3f; //0.2f;
    }
    arrFrustVerts[4] = passInfo.GetCamera().GetPosition();

    // 5 to 9 are the translated frustum vertices
    Vec3 vSunDir = (m_light.m_Origin - vCamPos).normalized() * Get3DEngine()->m_fSunClipPlaneRange;
    for (int v = 0; v < 5; v++)
    {
        arrFrustVerts[v + 5] = arrFrustVerts[v] + vSunDir;
    }

    // The method outputs at most 10 planes
    lstCastersHull.reserve(10);

    // Indices to create the planes of the camera frustum, can be offset by 5 to create planes for the translated frustum
    uint vertexIndex[5][3] = {
        {4, 1, 0}, {4, 0, 3}, {4, 3, 2}, {4, 2, 1}, {0, 1, 2}
    };

    Plane planeArray[5];
    planeArray[0] = Plane::CreatePlane(arrFrustVerts[vertexIndex[0][0]], arrFrustVerts[vertexIndex[0][1]], arrFrustVerts[vertexIndex[0][2]]);
    planeArray[1] = Plane::CreatePlane(arrFrustVerts[vertexIndex[1][0]], arrFrustVerts[vertexIndex[1][1]], arrFrustVerts[vertexIndex[1][2]]);
    planeArray[2] = Plane::CreatePlane(arrFrustVerts[vertexIndex[2][0]], arrFrustVerts[vertexIndex[2][1]], arrFrustVerts[vertexIndex[2][2]]);
    planeArray[3] = Plane::CreatePlane(arrFrustVerts[vertexIndex[3][0]], arrFrustVerts[vertexIndex[3][1]], arrFrustVerts[vertexIndex[3][2]]);
    planeArray[4] = Plane::CreatePlane(arrFrustVerts[vertexIndex[4][0]], arrFrustVerts[vertexIndex[4][1]], arrFrustVerts[vertexIndex[4][2]]);

    // Test each plane against the sun vector to know if all the translated vertices are on the correct side of the plane.
    bool bUsePlane[5];
    for (uint i = 0; i < 5; ++i)
    {
        bUsePlane[i] = planeArray[i].n.Dot(vSunDir) > 0;
    }

    // Select the far plane
    if (bUsePlane[4])
    {
        SPlaneObject po;
        po.plane = planeArray[4];
        po.Update();
        lstCastersHull.Add(po);
    }
    else
    {
        SPlaneObject po;
        po.plane = Plane::CreatePlane(arrFrustVerts[vertexIndex[4][0] + 5], arrFrustVerts[vertexIndex[4][1] + 5], arrFrustVerts[vertexIndex[4][2] + 5]);
        po.Update();
        lstCastersHull.Add(po);
    }

    // Select side planes
    for (uint i = 0; i < 4; ++i)
    {
        uint iPlaneOffset = bUsePlane[i] ? 0 : 5;
        uint iOtherOffset = bUsePlane[i] ? 5 : 0;
        uint iNextPlane = i < 3 ? i + 1 : 0;

        // Either add this plane or the equivalent plane in the translated frustum
        if (bUsePlane[i])
        {
            SPlaneObject po;
            po.plane = planeArray[i];
            po.Update();
            lstCastersHull.Add(po);
        }
        else
        {
            SPlaneObject po;
            po.plane = Plane::CreatePlane(arrFrustVerts[vertexIndex[i][0] + 5], arrFrustVerts[vertexIndex[i][1] + 5], arrFrustVerts[vertexIndex[i][2] + 5]);
            po.Update();
            lstCastersHull.Add(po);
        }

        // If this plane belong to a different frustum than the far plane, add a junction plane
        if (bUsePlane[4] != bUsePlane[i])
        {
            SPlaneObject po;
            po.plane = Plane::CreatePlane(arrFrustVerts[vertexIndex[i][1] + iPlaneOffset], arrFrustVerts[vertexIndex[i][1] + iOtherOffset], arrFrustVerts[vertexIndex[i][2] + iPlaneOffset]);
            //plane won't be valid in the case that the sun vector aligned with this edge of the camera frustrum. When this happens the new planes are pulled straight along that vector
            //and the 3 vertices used will form 2 colinear vectors. Which will create an invalid plane.
            //This means we don't need the junction plane however, so just don't create it.
            if (po.plane.IsValid())
            {
                po.Update();
                lstCastersHull.Add(po);
            }
        }

        // If this plane belong to a different frustum than the next plane, add a junction plane
        if (bUsePlane[iNextPlane] != bUsePlane[i])
        {
            SPlaneObject po;
            po.plane = Plane::CreatePlane(arrFrustVerts[vertexIndex[i][0] + iPlaneOffset], arrFrustVerts[vertexIndex[i][2] + iPlaneOffset], arrFrustVerts[vertexIndex[i][2] + iOtherOffset]);
            po.Update();
            lstCastersHull.Add(po);
        }
    }

    return lstCastersHull.Count();
}

void CLightEntity::FillFrustumCastersList_SUN(ShadowMapFrustum* pFr, int dwAllowedTypes, int nRenderNodeFlags, PodArray<SPlaneObject>& lstCastersHull, int nLod, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    pFr->bOmniDirectionalShadow = false;

    if (pFr->bBlendFrustum)
    {
        float fBlendVal = GetCVars()->e_ShadowsBlendCascadesVal;

        float fRange = Get3DEngine()->m_fGsmRange;
        float fRangeStep = Get3DEngine()->m_fGsmRangeStep;
        float fRadius  =  fRange * powf(fRangeStep, aznumeric_caster(nLod));

        float fBlendRadius = fRadius - (fBlendVal * (nLod + 1));

        pFr->fBlendVal =  fBlendRadius / fRadius;
        pFr->bBlendFrustum = true;
    }

    AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
    const AZ::Vector3 terrainCenter = terrainAabb.GetCenter();
    const float terrainSize = AZ::GetMax(terrainAabb.GetXExtent(), terrainAabb.GetYExtent());
    Vec3 vMapCenter = AZVec3ToLYVec3(terrainCenter);

    // prevent crash in qhull
    if (!dwAllowedTypes || !((passInfo.GetCamera().GetPosition() - vMapCenter).GetLength() < terrainSize * 4))
    {
        return;
    }

    if (!lstCastersHull.Count()) // make hull first time it is needed
    {
        MakeShadowCastersHullSun(lstCastersHull, passInfo);
    }

    if (pFr->isUpdateRequested(0))
    {
        pFr->ResetCasterLists();
        if (pFr->m_eFrustumType != ShadowMapFrustum::e_GsmDynamicDistance || GetCVars()->e_DynamicDistanceShadows > 0)
        {
            PodArray<SPlaneObject>* pShadowHull = (pFr->nShadowMapLod && !passInfo.IsRenderingCubemap()) ? &lstCastersHull : NULL;
#ifdef FEATURE_SVO_GI
            if (gEnv->pConsole->GetCVar("e_svoTI_Active") &&
                gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal())
            {
                pShadowHull = NULL; // TODO: enable hull usage for GI (use extended hull check)
            }
#endif
            m_pObjManager->MakeShadowCastersList((CVisArea*)GetEntityVisArea(), GetBBox(),
                dwAllowedTypes, nRenderNodeFlags, pFr->vLightSrcRelPos + pFr->vLightSrcRelPos, &m_light, pFr, pShadowHull, passInfo);
        }
    }
}

void CLightEntity::FillFrustumCastersList_PROJECTOR(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    //  fill casters list
    pFr->ResetCasterLists();
    pFr->bOmniDirectionalShadow = false;

    if (dwAllowedTypes)
    {
        // setup camera
        CCamera& FrustCam = pFr->FrustumPlanes[0] = CCamera();
        Vec3 vLightDir = -pFr->vLightSrcRelPos.normalized();
        Matrix34 mat = Matrix33::CreateRotationVDir(vLightDir);
        mat.SetTranslation(GetBBox().GetCenter());

        FrustCam.SetMatrix(mat);
        FrustCam.SetFrustum(pFr->nTexSize, pFr->nTexSize, pFr->fFOV * (gf_PI / 180.0f), pFr->fNearDist, pFr->fFarDist);

        m_pObjManager->MakeShadowCastersList((CVisArea*)GetEntityVisArea(), GetBBox(),
            dwAllowedTypes, 0xFFFFFFFF, pFr->vLightSrcRelPos + GetBBox().GetCenter(), &m_light, pFr, NULL, passInfo);

        DetectCastersListChanges(pFr, passInfo);

        pFr->aabbCasters.Reset(); // fix: should i .Reset() pFr->aabbCasters ?
    }
}

void CLightEntity::FillFrustumCastersList_OMNI(ShadowMapFrustum* pFr, int dwAllowedTypes, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    //  fill casters list
    pFr->ResetCasterLists();

    if (dwAllowedTypes)
    {
        // setup camera
        CCamera& FrustCam = pFr->FrustumPlanes[0] = CCamera();
        Vec3 vLightDir = -pFr->vLightSrcRelPos.normalized();
        Matrix34 mat = Matrix33::CreateRotationVDir(vLightDir);
        mat.SetTranslation(GetBBox().GetCenter());

        FrustCam.SetMatrix(mat);
        FrustCam.SetFrustum(256, 256, pFr->fFOV * (gf_PI / 180.0f) * 0.9f, pFr->fNearDist, pFr->fFarDist);

        m_pObjManager->MakeShadowCastersList((CVisArea*)GetEntityVisArea(), GetBBox(),
            dwAllowedTypes, 0xFFFFFFFF, pFr->vLightSrcRelPos + GetBBox().GetCenter(), &m_light, pFr, NULL, passInfo);

        DetectCastersListChanges(pFr, passInfo);

        pFr->aabbCasters.Reset(); // fix: should i .Reset() pFr->aabbCasters ?

        // Update all omni frustums
        pFr->UpdateOmniFrustums();
    }
}

void CLightEntity::DetectCastersListChanges(ShadowMapFrustum* pFr, const SRenderingPassInfo& passInfo)
{
    uint32 uCastersListCheckSum = 0;
    for (int i = 0; i < pFr->m_castersList.Count(); ++i)
    {
        IShadowCaster* pNode = pFr->m_castersList.GetAt(i);
        const AABB entBox = pNode->GetBBoxVirtual();
        uCastersListCheckSum += uint32((entBox.min.x + entBox.min.y + entBox.min.z) * 10000.f);
        uCastersListCheckSum += uint32((entBox.max.x + entBox.max.y + entBox.max.z) * 10000.f);
    }

    if (pFr->fRadius < DISTANCE_TO_THE_SUN)
    {
        uCastersListCheckSum += uint32((m_WSBBox.min.x + m_WSBBox.min.y + m_WSBBox.min.z) * 10000.f);
        uCastersListCheckSum += uint32((m_WSBBox.max.x + m_WSBBox.max.y + m_WSBBox.max.z) * 10000.f);
    }

    if (pFr->uCastersListCheckSum != uCastersListCheckSum)
    {
        pFr->RequestUpdate();
        pFr->uCastersListCheckSum = uCastersListCheckSum;

        if (GetCVars()->e_ShadowsDebug == 3)
        {
            const char* szName = ((CLightEntity*)(pFr->pLightOwner))->m_light.m_sName;
            PrintMessage("Requesting %s shadow update for %s, frame id = %d",
                pFr->bOmniDirectionalShadow ? "Cube" : "2D", szName, passInfo.GetFrameID());
        }
    }
}

ShadowMapFrustum* CLightEntity::GetShadowFrustum(int nId)
{
    if (m_pShadowMapInfo && nId < MAX_GSM_LODS_NUM)
    {
        return m_pShadowMapInfo->pGSM[nId];
    }

    return NULL;
};


void CLightEntity::OnCasterDeleted(IShadowCaster* pCaster)
{
    if (!m_pShadowMapInfo)
    {
        return;
    }

    for (int nGsmId = 0; nGsmId < MAX_GSM_LODS_NUM; nGsmId++)
    {
        ShadowMapFrustum* pFr = m_pShadowMapInfo->pGSM[nGsmId];
        if (pFr)
        {
            pFr->m_castersList.Delete(pCaster);
            pFr->m_jobExecutedCastersList.Delete(pCaster);

            if (pFr->pShadowCacheData)
            {
                pFr->pShadowCacheData->mProcessedCasters.erase(pCaster);
            }
        }
    }
}

void CLightEntity::GetMemoryUsage(ICrySizer* pSizer) const
{
    SIZER_COMPONENT_NAME(pSizer, "LightEntity");
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_pShadowMapInfo);
}

void CLightEntity::UpdateCastShadowFlag(float fDistance, const SRenderingPassInfo& passInfo)
{
    if (!(m_light.m_Flags & DLF_SUN))
    {
        if (fDistance > m_fWSMaxViewDist * GetFloatCVar(e_ShadowsCastViewDistRatioLights) || !passInfo.RenderShadows())
        {
            m_light.m_Flags &= ~DLF_CASTSHADOW_MAPS;
        }
        else if (m_bShadowCaster)
        {
            m_light.m_Flags |= DLF_CASTSHADOW_MAPS;
        }
    }

#if defined(FEATURE_SVO_GI)
    if (GetVoxelGIMode() == VM_Dynamic)
    {
        m_light.m_Flags |= DLF_USE_FOR_SVOGI;
    }
    else
    {
        m_light.m_Flags &= ~DLF_USE_FOR_SVOGI;
    }
#endif
}

void CLightEntity::Render(const SRendParams& rParams, const SRenderingPassInfo& passInfo)
{
#if defined(FEATURE_SVO_GI)
    if (gEnv->pConsole->GetCVar("e_svoTI_Active") && 
        gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal() && 
        gEnv->pConsole->GetCVar("e_GI")->GetIVal() &&
        !GetVoxelGIMode())
    {
        return;
    }
#endif
    if (m_layerId != uint16(~0) && m_dwRndFlags & ERF_HIDDEN)
    {
        return;
    }

    if (!(m_light.m_Flags & DLF_DEFERRED_LIGHT) || passInfo.IsRecursivePass())
    {
        return;
    }

    if (m_light.m_fRadius < 0.01f)
    {
        return;
    }

    UpdateCastShadowFlag(rParams.fDistance, passInfo);

    FUNCTION_PROFILER_3DENGINE;

    int nRenderNodeMinSpec = (m_dwRndFlags & ERF_SPEC_BITS_MASK) >> ERF_SPEC_BITS_SHIFT;
    if (!CheckMinSpec(nRenderNodeMinSpec))
    {
        return;
    }

    Sphere sp(m_light.m_BaseOrigin, m_light.m_fBaseRadius);

    bool bIsVisible = false;
    if (m_light.m_Flags & DLF_DEFERRED_CUBEMAPS)
    {
        OBB obb(OBB::CreateOBBfromAABB(Matrix33(m_light.m_ObjMatrix), AABB(-m_light.m_ProbeExtents, m_light.m_ProbeExtents)));
        bIsVisible = passInfo.GetCamera().IsOBBVisible_F(m_light.m_Origin, obb);
    }
    else if (m_light.m_Flags & DLF_AREA_LIGHT)
    {
        // OBB test for area lights.
        Vec3 vBoxMax(m_light.m_fBaseRadius, m_light.m_fBaseRadius + m_light.m_fAreaWidth, m_light.m_fBaseRadius + m_light.m_fAreaHeight);
        Vec3 vBoxMin(-0.1f, -(m_light.m_fBaseRadius + m_light.m_fAreaWidth), -(m_light.m_fBaseRadius + m_light.m_fAreaHeight));

        OBB obb(OBB::CreateOBBfromAABB(Matrix33(m_light.m_ObjMatrix), AABB(vBoxMin, vBoxMax)));
        bIsVisible = passInfo.GetCamera().IsOBBVisible_F(m_light.m_BaseOrigin, obb);
    }
    else
    {
        bIsVisible = passInfo.GetCamera().IsSphereVisible_F(sp);
    }

    if (!bIsVisible && !(m_light.m_Flags & DLF_ATTACH_TO_SUN))
    {
        return;
    }

    //assert(m_light.IsOk());

    if ((m_light.m_Flags & DLF_DISABLED) || (!GetCVars()->e_DynamicLights))
    {
        return;
    }

    if ((m_light.m_Flags & DLF_PROJECT) && (m_light.m_fLightFrustumAngle < 90.f) && (m_light.m_pLightImage))
    {
        if (!gEnv->pConsole->GetCVar("e_GI")->GetIVal() || 
            (gEnv->pConsole->GetCVar("e_svoTI_Active") && !gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal()) || 
            GetVoxelGIMode() != VM_Dynamic)
        {
            CCamera lightCam = passInfo.GetCamera();
            lightCam.SetPositionNoUpdate(m_light.m_Origin);
            Matrix34 entMat = ((ILightSource*)(m_light.m_pOwner))->GetMatrix();
            entMat.OrthonormalizeFast();
            Matrix33 matRot = Matrix33::CreateRotationVDir(entMat.GetColumn(0));
            lightCam.SetMatrixNoUpdate(Matrix34(matRot, m_light.m_Origin));
            lightCam.SetFrustum(1, 1, (m_light.m_fLightFrustumAngle * 2) / 180.0f * gf_PI, 0.1f, m_light.m_fRadius);
            if (!FrustumIntersection(passInfo.GetCamera(), lightCam))
            {
                return;
            }
        }
    }

    const int nEngineFrameID = passInfo.GetFrameID();

    int nMaxReqursion = (m_light.m_Flags & DLF_THIS_AREA_ONLY) ? 2 : 3;
    if (!m_pObjManager || !m_pVisAreaManager || !m_pVisAreaManager->IsEntityVisAreaVisible(this, nMaxReqursion, &m_light, passInfo))
    {
        if (m_light.m_Flags & (DLF_SUN) && m_pVisAreaManager && m_pVisAreaManager->m_bSunIsNeeded)
        {     // sun may be used in indoor even if outdoor is not visible
        }
        else if (!GetEntityVisArea() && !(m_light.m_Flags & DLF_THIS_AREA_ONLY) && m_pVisAreaManager && m_pVisAreaManager->m_bSunIsNeeded)
        {     // not "this area only" outdoor light affects everything
        }
        else if ((m_light.m_Flags & (DLF_IGNORES_VISAREAS | DLF_THIS_AREA_ONLY)) == DLF_IGNORES_VISAREAS)
        {
        }
        else
        {
            return;
        }
    }

    if (CVisArea* pArea = (CVisArea*)GetEntityVisArea())
    {     // vis area lsource
        IVisArea* pCameraVisArea = Get3DEngine()->GetVisAreaFromPos(passInfo.GetCamera().GetPosition());

        // check if light is visible thru light area portal cameras
        if (pArea->m_nRndFrameId == nEngineFrameID && pArea != (CVisArea*)pCameraVisArea)
        {
            int nCam = 0;
            for (; nCam < pArea->m_lstCurCamerasLen; nCam++)
            {
                if (CVisArea::s_tmpCameras[pArea->m_lstCurCamerasIdx + nCam].IsSphereVisible_F(sp))
                {
                    break;
                }
            }

            if (nCam == pArea->m_lstCurCamerasLen)
            {
                return;     // invisible
            }
        }

        // check if lsource is in visible area
        if (!IsLightAreasVisible() && pCameraVisArea != pArea)
        {
            if (m_light.m_Flags & DLF_THIS_AREA_ONLY)
            {
                if (pArea)
                {
                    int nRndFrameId = pArea->GetVisFrameId();
                    if (nEngineFrameID - nRndFrameId > MAX_FRAME_ID_STEP_PER_FRAME)
                    {
                        return;     // area invisible
                    }
                }
                else
                {
                    return;     // area invisible
                }
            }
        }
    }
    else
    {     // outdoor lsource
        if (!(m_light.m_Flags & DLF_DIRECTIONAL) && !IsLightAreasVisible())
        {
            return;     // outdoor invisible
        }
    }

    m_light.m_nStencilRef[0] = CClipVolumeManager::AffectsEverythingStencilRef;
    m_light.m_nStencilRef[1] = CClipVolumeManager::InactiveVolumeStencilRef;

    if (m_light.m_Flags & DLF_THIS_AREA_ONLY)
    {
        // User assigned clip volumes. Note: ClipVolume 0 has already been assigned in AsyncOctreeUpdate
        if ((m_light.m_Flags & DLF_HAS_CLIP_VOLUME) != 0 && m_light.m_pClipVolumes[1] != NULL)
        {
            m_light.m_nStencilRef[1] = m_light.m_pClipVolumes[1]->GetStencilRef();
        }

        m_light.m_nStencilRef[0] = (m_pRNTmpData && m_pRNTmpData->userData.m_pClipVolume) ?  m_pRNTmpData->userData.m_pClipVolume->GetStencilRef() : 0;
    }

    // associated clip volume invisible
    if (m_light.m_nStencilRef[0] == CClipVolumeManager::InactiveVolumeStencilRef && m_light.m_nStencilRef[1] == CClipVolumeManager::InactiveVolumeStencilRef)
    {
        return;
    }

    _smart_ptr<IMaterial> pMat = GetMaterial();
    if (pMat)
    {
        SAFE_RELEASE(m_light.m_Shader.m_pShader);
        m_light.m_Shader = pMat->GetShaderItem();
        if (m_light.m_Shader.m_pShader)
        {
            m_light.m_Shader.m_pShader->AddRef();
        }
    }

    GetRenderer()->EF_UpdateDLight(&m_light);

    int16 forwardLightCount = Get3DEngine()->GetSunEntity() ? 1 : 0;
    m_light.m_Id = int16(forwardLightCount + GetRenderer()->EF_GetDeferredLightsNum());

    bool castShadows = false;
    if (passInfo.RenderShadows() && (m_light.m_Flags & DLF_CASTSHADOW_MAPS) && m_light.m_Id >= 0)
    {
        UpdateGSMLightSourceShadowFrustum(passInfo);

        if (m_pShadowMapInfo)
        {
            m_light.m_pShadowMapFrustums = m_pShadowMapInfo->pGSM;
        }

        castShadows = true;
    }

    if (GetCVars()->e_DynamicLights && m_fWSMaxViewDist)
    {
        if (GetCVars()->e_DynamicLights == 2)
        {
            CDLight* pL = &m_light;
            float fSize = 0.05f * (sinf(GetCurTimeSec() * 10.f) + 2.0f);
            DrawSphere(pL->m_Origin, fSize, pL->m_Color);
            GetRenderer()->DrawLabel(pL->m_Origin, 1.3f, "id=%d, rad=%.1f, vdm=%.1f, mvd=%.1f, shadows=%i", pL->m_Id, pL->m_fRadius, m_fViewDistanceMultiplier, m_fWSMaxViewDist, castShadows);
        }

        const float mult = SATURATE(6.f * (1.f - (rParams.fDistance / m_fWSMaxViewDist)));
        IF (m_light.m_Color.Luminance() * mult > 0, 1)
        {
            Get3DEngine()->AddLightToRenderer(m_light, mult, passInfo, SRendItemSorter(rParams.rendItemSorter));
        }
    }
}

IRenderNode::EVoxelGIMode CLightEntity::GetVoxelGIMode()
{
        if (m_light.m_BaseColor.Luminance() > .01f && m_light.m_fBaseRadius > 0.5f)
        {
            if (m_light.m_Flags & DLF_SUN)
            {
                if (GetCVars()->e_Sun)
                {
                    return VM_Static;
                }
                else
                {
                    return VM_None;
                }
            }
            return m_VoxelGIMode;
        }
    return VM_None;
}

void CLightEntity::SetDesiredVoxelGIMode(EVoxelGIMode mode)
{
    m_VoxelGIMode = mode;
}

void CLightEntity::SetName(const char* name)
{
    m_Name = name;

#if defined(FEATURE_SVO_GI)
    if (strstr(m_Name.c_str(), "_TI"))
    {
        if (strstr(m_Name.c_str(), "_TI_DYN"))
        {
            m_VoxelGIMode = VM_Dynamic;
        }
        else
        {
            m_VoxelGIMode = VM_Static;
        }
    }
#endif
}

void CLightEntity::OffsetPosition(const Vec3& delta)
{
    if (m_pRNTmpData)
    {
        m_pRNTmpData->OffsetPosition(delta);
    }
    m_light.m_Origin += delta;
    m_light.m_BaseOrigin += delta;
    m_Matrix.SetTranslation(m_Matrix.GetTranslation() + delta);
    m_WSBBox.Move(delta);
}

void CLightEntity::ProcessPerObjectFrustum(ShadowMapFrustum* pFr, struct SPerObjectShadow* pPerObjectShadow, ILightSource* pLightSource, const SRenderingPassInfo& passInfo)
{
    assert(pFr);
    CDLight& light = pLightSource->GetLightProperties();

    pFr->RequestUpdate();
    pFr->ResetCasterLists();
    pFr->m_castersList.Add(pPerObjectShadow->pCaster);

    // get caster's bounding box and scale
    AABB objectBBox;
    pPerObjectShadow->pCaster->FillBBox(objectBBox);
    Vec3 vExtents = 0.5f * objectBBox.GetSize().CompMul(pPerObjectShadow->vBBoxScale);
    pFr->aabbCasters = AABB(objectBBox.GetCenter() - vExtents, objectBBox.GetCenter() + vExtents);

    pFr->m_Flags = light.m_Flags;
    pFr->bUseAdditiveBlending = true;
    uint nTexSize = aznumeric_caster(pPerObjectShadow->nTexSize * GetCVars()->e_ShadowsPerObjectResolutionScale);
    nTexSize = clamp_tpl<uint>(nTexSize, 64, GetRenderer()->GetMaxTextureSize());
    pFr->nTexSize = 1 << IntegerLog2(nTexSize);
    pFr->nTextureWidth = pFr->nTexSize;
    pFr->nTextureHeight = pFr->nTexSize;
    pFr->bBlendFrustum = false;

    // now update frustum params based on object box
    AABB objectsBox = pFr->aabbCasters;
    const Vec3 lightPos = light.m_Origin - passInfo.GetCamera().GetPosition() + objectsBox.GetCenter();
    const Vec3 lookAt = objectsBox.GetCenter();

    pFr->vProjTranslation = objectsBox.GetCenter();
    pFr->vLightSrcRelPos = light.m_Origin - passInfo.GetCamera().GetPosition();
    pFr->fFOV = (float)RAD2DEG(atan_tpl(objectsBox.GetRadius() / (lookAt - lightPos).GetLength())) * 2.0f;
    pFr->fProjRatio = 1.0f;
    pFr->fNearDist = pFr->vLightSrcRelPos.GetLength() - objectsBox.GetRadius();
    pFr->fFarDist = pFr->vLightSrcRelPos.GetLength() + objectsBox.GetRadius();

    pFr->fDepthConstBias = pPerObjectShadow->fConstBias;
    pFr->fDepthSlopeBias = pPerObjectShadow->fSlopeBias;
    pFr->fWidthS = pPerObjectShadow->fJitter;
    pFr->fWidthT = pPerObjectShadow->fJitter;
    pFr->fBlurS  = 0.0f;
    pFr->fBlurT  = 0.0f;

    if (GetCVars()->e_ShadowsFrustums)
    {
        pFr->DrawFrustum(GetRenderer(), (GetCVars()->e_ShadowsFrustums == 1) ? 1000 : 1);
        Get3DEngine()->DrawBBox(pFr->aabbCasters, Col_Green);
    }
}
