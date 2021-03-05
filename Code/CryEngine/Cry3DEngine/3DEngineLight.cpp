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

// Description : Light sources manager


#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "AABBSV.h"
#include "LightEntity.h"
#include "ObjectsTree.h"
#include "ClipVolumeManager.h"

ILightSource* C3DEngine::CreateLightSource()
{
    // construct new object
    CLightEntity* pLightEntity = new CLightEntity();

    m_lstStaticLights.Add(pLightEntity);

    return pLightEntity;
}

void C3DEngine::DeleteLightSource(ILightSource* pLightSource)
{
    if (m_lstStaticLights.Delete((CLightEntity*)pLightSource) || pLightSource == m_pSun)
    {
        if (pLightSource == m_pSun)
        {
            m_pSun = NULL;
        }

        delete pLightSource;
    }
    else
    {
        assert(!"Light object not found");
    }
}

void CLightEntity::Release(bool)
{
    Get3DEngine()->UnRegisterEntityDirect(this);
    Get3DEngine()->DeleteLightSource(this);
}

void CLightEntity::SetLightProperties(const CDLight& light)
{
    C3DEngine* engine = Get3DEngine();

    m_light = light;

    m_bShadowCaster = (m_light.m_Flags & DLF_CASTSHADOW_MAPS) != 0;

    m_light.m_fBaseRadius = m_light.m_fRadius;
    m_light.m_fLightFrustumAngle = CLAMP(m_light.m_fLightFrustumAngle, 0.f, (LIGHT_PROJECTOR_MAX_FOV / 2.f));

    if (!(m_light.m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT)))
    {
        m_light.m_fLightFrustumAngle = 90.f / 2.f;
    }

    m_light.m_pOwner = this;

    if (m_light.m_Flags & DLF_ATTACH_TO_SUN)
    {
        m_dwRndFlags |= ERF_RENDER_ALWAYS | ERF_HUD;
    }

    engine->GetLightEntities()->Delete((ILightSource*)this);

    PodArray<ILightSource*>& lightEntities = *engine->GetLightEntities();

    //on consoles we force all lights (except sun) to be deferred
    if (GetCVars()->e_DynamicLightsForceDeferred && !(m_light.m_Flags & (DLF_SUN | DLF_POST_3D_RENDERER)))
    {
        m_light.m_Flags |= DLF_DEFERRED_LIGHT;
    }

    if (light.m_Flags & DLF_DEFERRED_LIGHT)
    {
        lightEntities.Add((ILightSource*)this);
    }
    else
    {
        lightEntities.InsertBefore((ILightSource*)this, 0);
    }
}

void C3DEngine::ResetCasterCombinationsCache()
{
    for (int nSunInUse = 0; nSunInUse < 2; nSunInUse++)
    {
        // clear user counters
        for (ShadowFrustumListsCacheUsers::iterator it = m_FrustumsCacheUsers[nSunInUse].begin(); it != m_FrustumsCacheUsers[nSunInUse].end(); ++it)
        {
            it->second = 0;
        }
    }
}

void C3DEngine::DeleteAllStaticLightSources()
{
    for (int i = 0; i < m_lstStaticLights.Count(); i++)
    {
        delete m_lstStaticLights[i];
    }
    m_lstStaticLights.Reset();

    m_pSun = NULL;
}

void C3DEngine::InitShadowFrustums(const SRenderingPassInfo& passInfo)
{
    assert(passInfo.IsGeneralPass());
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    if (m_pSun)
    {
        CDLight* pLight = &m_pSun->GetLightProperties();
        CLightEntity* pLightEntity = (CLightEntity*)pLight->m_pOwner;

        if (passInfo.RenderShadows() && (pLight->m_Flags & DLF_CASTSHADOW_MAPS) && pLight->m_Id >= 0)
        {
            pLightEntity->UpdateGSMLightSourceShadowFrustum(passInfo);

            if (pLightEntity->m_pShadowMapInfo)
            {
                pLight->m_pShadowMapFrustums = pLightEntity->m_pShadowMapInfo->pGSM;
            }
        }

        _smart_ptr<IMaterial> pMat = pLightEntity->GetMaterial();
        if (pMat)
        {
            pLight->m_Shader = pMat->GetShaderItem();
        }

        // update copy of light ion the renderer
        if (pLight->m_Id >= 0)
        {
            CDLight* pRndLight = NULL;
            GetRenderer()->EF_Query(EFQ_LightSource, pLight->m_Id, pRndLight);
            assert(pLight->m_Id == pRndLight->m_Id);
            pRndLight->m_pShadowMapFrustums = pLight->m_pShadowMapFrustums;
            pRndLight->m_Shader = pLight->m_Shader;
            pRndLight->m_Flags = pLight->m_Flags;
        }

        // add per object shadow frustums
        m_nCustomShadowFrustumCount = 0;
        if (passInfo.RenderShadows() &&  GetCVars()->e_ShadowsPerObject > 0)
        {
            const uint nFrustumCount = m_lstPerObjectShadows.size();
            if (nFrustumCount > m_lstCustomShadowFrustums.size())
            {
                m_lstCustomShadowFrustums.resize(nFrustumCount);
            }

            for (uint i = 0; i < nFrustumCount; ++i)
            {
                if (m_lstPerObjectShadows[i].pCaster)
                {
                    ShadowMapFrustum* pFr = &m_lstCustomShadowFrustums[i];
                    pFr->m_eFrustumType = ShadowMapFrustum::e_PerObject;

                    CLightEntity::ProcessPerObjectFrustum(pFr, &m_lstPerObjectShadows[i], m_pSun, passInfo);
                    ++m_nCustomShadowFrustumCount;
                }
            }
        }
    }

    if (passInfo.RenderShadows())
    {
        ResetCasterCombinationsCache();
    }
}

void C3DEngine::AddPerObjectShadow(IShadowCaster* pCaster, float fConstBias, float fSlopeBias, float fJitter, const Vec3& vBBoxScale, uint nTexSize)
{
    SPerObjectShadow* pOS = GetPerObjectShadow(pCaster);
    if (!pOS)
    {
        pOS = &m_lstPerObjectShadows.AddNew();
    }

    pOS->pCaster = pCaster;
    pOS->fConstBias = fConstBias;
    pOS->fSlopeBias = fSlopeBias;
    pOS->fJitter = fJitter;
    pOS->vBBoxScale = vBBoxScale;
    pOS->nTexSize = nTexSize;
}

void C3DEngine::RemovePerObjectShadow(IShadowCaster* pCaster)
{
    SPerObjectShadow* pOS = GetPerObjectShadow(pCaster);
    if (pOS)
    {
        FRAME_PROFILER("C3DEngine::RemovePerObjectShadow", GetSystem(), PROFILE_3DENGINE);

        size_t nIndex = (size_t)(pOS - m_lstPerObjectShadows.begin());
        m_lstPerObjectShadows.Delete(nIndex);
    }
}

struct SPerObjectShadow* C3DEngine::GetPerObjectShadow(IShadowCaster* pCaster)
{
    for (int i = 0; i < m_lstPerObjectShadows.Count(); ++i)
    {
        if (m_lstPerObjectShadows[i].pCaster == pCaster)
        {
            return &m_lstPerObjectShadows[i];
        }
    }

    return NULL;
}

void C3DEngine::GetCustomShadowMapFrustums(ShadowMapFrustum*& arrFrustums, int& nFrustumCount)
{
    arrFrustums = m_lstCustomShadowFrustums.begin();
    nFrustumCount = m_nCustomShadowFrustumCount;
}

//  delete pLight->m_pProjCamera;
//pLight->m_pProjCamera=0;
//if(pLight->m_pShader)
//      SAFE_RELEASE(pLight->m_pShader);
namespace
{
    static inline bool CmpCastShadowFlag(const CDLight* p1, const CDLight* p2)
    {
        // move sun first
        if ((p1->m_Flags & DLF_SUN) > (p2->m_Flags & DLF_SUN))
        {
            return true;
        }
        else if ((p1->m_Flags & DLF_SUN) < (p2->m_Flags & DLF_SUN))
        {
            return false;
        }

        // move shadow casters first
        if ((p1->m_Flags & DLF_CASTSHADOW_MAPS) > (p2->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            return true;
        }
        else if ((p1->m_Flags & DLF_CASTSHADOW_MAPS) < (p2->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            return false;
        }

        // get some sorting consistency for shadow casters
        if (p1->m_pOwner > p2->m_pOwner)
        {
            return true;
        }
        else if (p1->m_pOwner < p2->m_pOwner)
        {
            return false;
        }

        return false;
    }
}

//////////////////////////////////////////////////////////////////////////

void C3DEngine::SubmitSun(const SRenderingPassInfo& passInfo)
{
    assert(passInfo.IsGeneralPass());
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    if (m_pSun)
    {
        CDLight* light = &m_pSun->GetLightProperties();

        GetRenderer()->EF_ADDDlight(light, passInfo);
    }
}

void C3DEngine::RemoveEntityLightSources(IRenderNode* pEntity)
{
    for (int i = 0; i < m_lstStaticLights.Count(); i++)
    {
        if (m_lstStaticLights[i] == pEntity)
        {
            m_lstStaticLights.Delete(i);
            if (pEntity == m_pSun)
            {
                m_pSun = NULL;
            }
            i--;
        }
    }
}

ILightSource* C3DEngine::GetSunEntity()
{
    return m_pSun;
}

void C3DEngine::OnCasterDeleted(IShadowCaster* pCaster)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_3DENGINE);
    { // make sure pointer to object will not be used somewhere in the renderer
        if (m_pSun)
        {
            m_pSun->OnCasterDeleted(pCaster);
        }

        if (GetRenderer()->GetActiveGPUCount() > 1)
        {
            if (ShadowFrustumMGPUCache* pFrustumCache = GetRenderer()->GetShadowFrustumMGPUCache())
            {
                pFrustumCache->DeleteFromCache(pCaster);
            }
        }

        // remove from per object shadows list
        RemovePerObjectShadow(pCaster);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Init()
{
    m_bUpdateLightVolumes = false;
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_pLightVolumes[i].reserve(LV_MAX_COUNT);
        m_pLightVolsInfo[i].reserve(LV_MAX_COUNT);
    }
    memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
    memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));
}

void CLightVolumesMgr::Reset()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        stl::free_container(m_pLightVolumes[i]);
    }

    m_bUpdateLightVolumes = false;
    memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
    memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));
}

//////////////////////////////////////////////////////////////////////////

uint16 CLightVolumesMgr::RegisterVolume(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    IF ((m_bUpdateLightVolumes && (lightVolsInfo.size() < LV_MAX_COUNT)) && fRadius < 256.0f, 1)
    {
        FUNCTION_PROFILER_3DENGINE;

        int32 nPosx = (int32)(floorf(vPos.x * LV_CELL_RSIZEX));
        int32 nPosy = (int32)(floorf(vPos.y * LV_CELL_RSIZEY));
        int32 nPosz = (int32)(floorf(vPos.z * LV_CELL_RSIZEZ));

        // Check if world cell has any light volume, else add new one
        uint16 nHashIndex = GetWorldHashBucketKey(nPosx, nPosy, nPosz);
        uint16* pCurrentVolumeID = &m_nWorldCells[nHashIndex];

        while (*pCurrentVolumeID != 0)
        {
            SLightVolInfo& sVolInfo = *lightVolsInfo[*pCurrentVolumeID - 1];

            int32 nVolumePosx = (int32)(floorf(sVolInfo.vVolume.x * LV_CELL_RSIZEX));
            int32 nVolumePosy = (int32)(floorf(sVolInfo.vVolume.y * LV_CELL_RSIZEY));
            int32 nVolumePosz = (int32)(floorf(sVolInfo.vVolume.z * LV_CELL_RSIZEZ));

            if (nPosx == nVolumePosx &&
                nPosy == nVolumePosy &&
                nPosz == nVolumePosz &&
                nClipVolumeRef  == sVolInfo.nClipVolumeID)
            {
                return (uint16) * pCurrentVolumeID;
            }

            pCurrentVolumeID = &sVolInfo.nNextVolume;
        }

        // create new volume
        SLightVolInfo* pLightVolInfo = new SLightVolInfo(vPos, fRadius, nClipVolumeRef);
        lightVolsInfo.push_back(pLightVolInfo);
        *pCurrentVolumeID = lightVolsInfo.size();

        return *pCurrentVolumeID;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::RegisterLight(const CDLight& pDL, uint32 nLightID, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
    IF ((m_bUpdateLightVolumes && !(pDL.m_Flags & LV_DLF_LIGHTVOLUMES_MASK)), 1)
    {
        FUNCTION_PROFILER_3DENGINE;

        const f32 fColCheck = (f32) fsel(pDL.m_Color.r + pDL.m_Color.g + pDL.m_Color.b - 0.333f, 1.0f, 0.0f);  //light color > threshold
        const f32 fRadCheck = (f32) fsel(pDL.m_fRadius - 0.5f, 1.0f, 0.0f);  //light radius > threshold
        if (fColCheck * fRadCheck)
        {
            //if the radius is large than certain value, all the the world light cells will be lighted anyway. So we just add the light to all the cells
            //the input radius restriction will be added too
            if(floorf(pDL.m_fRadius*LV_LIGHT_CELL_R_SIZE) > LV_LIGHTS_WORLD_BUCKET_SIZE)
            {
                for (int32 idx = 0; idx < LV_LIGHTS_WORLD_BUCKET_SIZE; idx++)
                {
                    SLightCell& lightCell = m_pWorldLightCells[idx];
                    CryPrefetch(&lightCell);
                    if (lightCell.nLightCount < LV_LIGHTS_MAX_COUNT)
                    {
                        lightCell.nLightID[lightCell.nLightCount] = nLightID;
                        lightCell.nLightCount += 1;
                    }
                }
            }
            else
            {
                int32 nMiny = (int32)(floorf((pDL.m_Origin.y - pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMaxy = (int32)(floorf((pDL.m_Origin.y + pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMinx = (int32)(floorf((pDL.m_Origin.x - pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));
                int32 nMaxx = (int32)(floorf((pDL.m_Origin.x + pDL.m_fRadius) * LV_LIGHT_CELL_R_SIZE));

                // Register light into all cells touched by light radius
                for (int32 y = nMiny, ymax = nMaxy; y <= ymax; ++y)
                {
                    for (int32 x = nMinx, xmax = nMaxx; x <= xmax; ++x)
                    {
                        SLightCell& lightCell = m_pWorldLightCells[GetWorldHashBucketKey(x, y, 1, LV_LIGHTS_WORLD_BUCKET_SIZE)];
                        CryPrefetch(&lightCell);
                        if (lightCell.nLightCount < LV_LIGHTS_MAX_COUNT)
                        {
                            //only if the las light added to the cell wasn't the same light
                            if (!(lightCell.nLightCount > 0 && lightCell.nLightID[lightCell.nLightCount - 1] == nLightID))
                            {
                                lightCell.nLightID[lightCell.nLightCount] = nLightID;
                                lightCell.nLightCount += 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::AddLight(const SRenderLight& pLight, const SLightVolInfo* __restrict pVolInfo, SLightVolume& pVolume)
{
    // Check for clip volume
    if (pLight.m_nStencilRef[0] == pVolInfo->nClipVolumeID || pLight.m_nStencilRef[1] == pVolInfo->nClipVolumeID ||
        pLight.m_nStencilRef[0] == CClipVolumeManager::AffectsEverythingStencilRef)
    {
        const Vec4* __restrict vLight = (Vec4*) &pLight.m_Origin.x;
        const Vec4& vVolume = pVolInfo->vVolume;
        const f32 fDimCheck = (f32) fsel(vLight->w - vVolume.w * 0.1f, 1.0f, 0.0f);  //light radius not more than 10x smaller than volume radius
        const f32 fOverlapCheck = (f32) fsel(sqr(vVolume.x - vLight->x) + sqr(vVolume.y - vLight->y) + sqr(vVolume.z - vLight->z) - sqr(vVolume.w + vLight->w), 0.0f, 1.0f);// touches volumes
        if (fDimCheck * fOverlapCheck)
        {
            float fAttenuationBulbSize = pLight.m_fAttenuationBulbSize;
            Vec3 lightColor =  *((Vec3*)&pLight.m_Color);

            // Adjust light intensity so that the intended brightness is reached 1 meter from the light's surface
            IF (!(pLight.m_Flags & (DLF_AREA_LIGHT | DLF_AMBIENT)), 1)
            {
                fAttenuationBulbSize = max(fAttenuationBulbSize, 0.001f);

                // Solve I * 1 / (1 + d/lightsize)^2 = 1
                float intensityMul = 1.0f + 1.0f / fAttenuationBulbSize;
                intensityMul *= intensityMul;
                lightColor *= intensityMul;
            }

            pVolume.pData.push_back();
            SLightVolume::SLightData& lightData = pVolume.pData[pVolume.pData.size() - 1];
            lightData.vPos = *vLight;
            lightData.vColor = Vec4(lightColor, fAttenuationBulbSize);
            lightData.vParams = Vec4(0.f, 0.f, 0.f, 0.f);

            IF (pLight.m_Flags & DLF_PROJECT, 1)
            {
                lightData.vParams = Vec4(pLight.m_ObjMatrix.GetColumn0(), cos_tpl(DEG2RAD(pLight.m_fLightFrustumAngle)));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Update(const SRenderingPassInfo& passInfo)
{
    uint32 nThreadID = passInfo.ThreadID();
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[nThreadID];

    if (!m_bUpdateLightVolumes || lightVolsInfo.empty())
    {
        return;
    }

    FUNCTION_PROFILER_3DENGINE;
    TArray<SRenderLight>* pLights = GetRenderer()->EF_GetDeferredLights(passInfo);
    const uint32 nLightCount = pLights->size();

    uint32 nLightVols = lightVolsInfo.size();
    LightVolumeVector& lightVols = m_pLightVolumes[nThreadID];
    uint32 existingLightVolsCount = 0;  //This is 0, that just means we will be overwriting all existing light volumes
    
    //If this is a recursive pass (not the first time that this is called this frame), we're just going to be adding on new light volumes to the existing collection
    if (passInfo.IsRecursivePass())
    {
        existingLightVolsCount = lightVols.size();

        //If no new light volumes have been added, don't bother updating
        if (nLightVols == existingLightVolsCount)
        {
            return;
        }
    }

    lightVols.resize(nLightVols);

    if (!nLightCount)
    {
        //Start out existingLightVolsCount to avoid clearing out existing light volumes when we don't need to
        for (uint32 v = existingLightVolsCount; v < nLightVols; ++v)
        {
            lightVols[v].pData.resize(0);
        }

        return;
    }

    const int MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE = 1024;

    if (nLightCount > MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE)
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "More lights in the scene (%d) than supported by the Light Volume Update function (%d). Extra lights will be ignored.",
            nLightCount, MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE);
    }

    //This can be a uint8 array because nLightVols should never be greater than 256(LV_MAX_COUNT)
    assert(LV_MAX_COUNT <= 256);
    uint8 lightProcessedStateArray[MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE];

    //Start at the number of light volumes that already exist so that we don't end up re-updating light volumes unnecessarily. 
    for (uint32 v = existingLightVolsCount; v < nLightVols; ++v)
    {
        const Vec4* __restrict vBVol = &lightVolsInfo[v]->vVolume;
        int32 nMiny = (int32)(floorf((vBVol->y - vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMaxy = (int32)(floorf((vBVol->y + vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMinx = (int32)(floorf((vBVol->x - vBVol->w) * LV_LIGHT_CELL_R_SIZE));
        int32 nMaxx = (int32)(floorf((vBVol->x + vBVol->w) * LV_LIGHT_CELL_R_SIZE));

        lightVols[v].pData.resize(0);

        // Loop through active light cells touching bounding volume (~avg 2 cells)
        for (int32 y = nMiny, ymax = nMaxy; y <= ymax; ++y)
        {
            for (int32 x = nMinx, xmax = nMaxx; x <= xmax; ++x)
            {
                const SLightCell& lightCell = m_pWorldLightCells[GetWorldHashBucketKey(x, y, 1, LV_LIGHTS_WORLD_BUCKET_SIZE)];
                CryPrefetch(&lightCell);

                const SRenderLight& pFirstDL = (*pLights)[lightCell.nLightID[0]];
                CryPrefetch(&pFirstDL);
                CryPrefetch(&pFirstDL.m_ObjMatrix);
                for (uint32 l = 0; (l < lightCell.nLightCount) & (lightVols[v].pData.size() < LIGHTVOLUME_MAXLIGHTS); ++l)
                {
                    const int32 nLightId = lightCell.nLightID[l];

                    //Only allow IDs < MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE to continue or else we'll overflow access to
                    //lightProcessedStateArray[MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE].  Skipping the extra lights shouldn't really matter
                    //since A) folks won't be using that many lights, and B) for the case of light emitting particles, they tend to be grouped
                    //so that the individual contributions tend to bleed together anyway.
                    if (nLightId >= MAX_NUM_LIGHTS_FOR_LIGHT_VOLUME_UPDATE)
                    {
                        continue;
                    }

                    if (static_cast<uint32>(nLightId) < nLightCount)
                    {
                        const SRenderLight& pDL = (*pLights)[nLightId];
                        const int32 nNextLightId = lightCell.nLightID[(l + 1) & (LIGHTVOLUME_MAXLIGHTS - 1)];
                        const SRenderLight& pNextDL = (*pLights)[nNextLightId];
                        CryPrefetch(&pNextDL);
                        CryPrefetch(&pNextDL.m_ObjMatrix);

                        IF(lightProcessedStateArray[nLightId] != v + 1, 1)
                        {
                            lightProcessedStateArray[nLightId] = v + 1;
                            AddLight(pDL, &*lightVolsInfo[v], lightVols[v]);
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::Clear(const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    m_bUpdateLightVolumes = false;
    if (GetCVars()->e_LightVolumes && passInfo.IsGeneralPass() && GetCVars()->e_DynamicLights)
    {
        memset(m_nWorldCells, 0, sizeof(m_nWorldCells));
        memset(m_pWorldLightCells, 0, sizeof(m_pWorldLightCells));

        //Clean up volume info data
        for (size_t i = 0; i < lightVolsInfo.size(); ++i)
        {
            delete lightVolsInfo[i];
        }

        m_pLightVolsInfo[passInfo.ThreadID()].clear();
        m_bUpdateLightVolumes = (GetCVars()->e_LightVolumes == 1) ? true : false;
    }
}

//////////////////////////////////////////////////////////////////////////

void CLightVolumesMgr::GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols)
{
    pLightVols = 0;
    nNumVols = 0;
    if (GetCVars()->e_LightVolumes == 1 && GetCVars()->e_DynamicLights && !m_pLightVolumes[nThreadID].empty())
    {
        pLightVols = &m_pLightVolumes[nThreadID][0];
        nNumVols = m_pLightVolumes[nThreadID].size();
    }
}


void C3DEngine::GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols)
{
    m_LightVolumesMgr.GetLightVolumes(nThreadID, pLightVols, nNumVols);
}

uint16 C3DEngine::RegisterVolumeForLighting(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo)
{
    return m_LightVolumesMgr.RegisterVolume(vPos, fRadius, nClipVolumeRef, passInfo);
}

//////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
void CLightVolumesMgr::DrawDebug(const SRenderingPassInfo& passInfo)
{
    DynArray<SLightVolInfo*>& lightVolsInfo = m_pLightVolsInfo[passInfo.ThreadID()];

    IRenderer* pRenderer = GetRenderer();
    IRenderAuxGeom* pAuxGeom = GetRenderer()->GetIRenderAuxGeom();
    if (!pAuxGeom || !passInfo.IsGeneralPass())
    {
        return;
    }

    ColorF cWhite = ColorF(1, 1, 1, 1);
    ColorF cBad = ColorF(1.0f, 0.0, 0.0f, 1.0f);
    ColorF cWarning = ColorF(1.0f, 1.0, 0.0f, 1.0f);
    ColorF cGood = ColorF(0.0f, 0.5, 1.0f, 1.0f);
    ColorF cSingleCell = ColorF(0.0f, 1.0, 0.0f, 1.0f);

    const uint32 nLightVols = lightVolsInfo.size();
    LightVolumeVector& lightVols = m_pLightVolumes[passInfo.ThreadID()];
    const Vec3 vCamPos = passInfo.GetCamera().GetPosition();

    float fYLine = 8.0f, fYStep = 20.0f;
    GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, (float*)&cWhite.r, false, "Light Volumes Info (count %d)", nLightVols);

    for (uint32 v = 0; v < nLightVols; ++v)  // draw each light volume
    {
        SLightVolume& lv = lightVols[v];
        SLightVolInfo& lvInfo = *lightVolsInfo[v];

        ColorF& cCol = (lv.pData.size() >= 10) ? cBad : ((lv.pData.size() >= 5) ? cWarning : cGood);
        const Vec3 vPos = Vec3(lvInfo.vVolume.x, lvInfo.vVolume.y, lvInfo.vVolume.z);
        const float fCamDistSq = (vPos - vCamPos).len2();
        cCol.a = max(0.25f, min(1.0f, 1024.0f / (fCamDistSq + 1e-6f)));

        pRenderer->DrawLabelEx(vPos, 1.3f, (float*)&cCol.r, true, true, "Id: %d\nPos: %.2f %.2f %.2f\nRadius: %.2f\nLights: %d\nOutLights: %d",
            v, vPos.x, vPos.y, vPos.z, lvInfo.vVolume.w, lv.pData.size(), (*(int32*)&lvInfo.vVolume.w) & (1 << 31) ? 1 : 0);

        if (GetCVars()->e_LightVolumesDebug == 2)
        {
            const float fSideSize = 0.707f * sqrtf(lvInfo.vVolume.w * lvInfo.vVolume.w * 2);
            pAuxGeom->DrawAABB(AABB(vPos - Vec3(fSideSize), vPos + Vec3(fSideSize)), false, cCol, eBBD_Faceted);
        }

        if (GetCVars()->e_LightVolumesDebug == 3)
        {
            cBad.a = 1.0f;
            const Vec3 vCellPos = Vec3(floorf((lvInfo.vVolume.x) * LV_CELL_RSIZEX) * LV_CELL_SIZEX,
                    floorf((lvInfo.vVolume.y) * LV_CELL_RSIZEY) * LV_CELL_SIZEY,
                    floorf((lvInfo.vVolume.z) * LV_CELL_RSIZEZ) * LV_CELL_SIZEZ);

            const Vec3 vMin = vCellPos;
            const Vec3 vMax = vMin + Vec3(LV_CELL_SIZEX, LV_CELL_SIZEY, LV_CELL_SIZEZ);
            pAuxGeom->DrawAABB(AABB(vMin, vMax), false, cBad, eBBD_Faceted);
        }
    }
}
#endif
