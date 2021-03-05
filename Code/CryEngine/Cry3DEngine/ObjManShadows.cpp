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

// Description : Shadow casters/receivers relations


#include "Cry3DEngine_precompiled.h"

#include "ObjMan.h"
#include "VisAreas.h"
#include "3dEngine.h"
#include <AABBSV.h>
#include "LightEntity.h"
#include "ObjectsTree.h"

bool IsAABBInsideHull(const SPlaneObject* pHullPlanes, int nPlanesNum, const AABB& aabbBox);

void CObjManager::MakeShadowCastersList(CVisArea* pArea, [[maybe_unused]] const AABB& aabbReceiver, [[maybe_unused]] int dwAllowedTypes, int32 nRenderNodeFlags, [[maybe_unused]] Vec3 vLightPos, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<SPlaneObject>* pShadowHull, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    assert(pLight && vLightPos.len() > 1); // world space pos required

    pFr->m_castersList.Clear();
    pFr->m_jobExecutedCastersList.Clear();

    CVisArea* pLightArea = pLight->m_pOwner ? (CVisArea*)pLight->m_pOwner->GetEntityVisArea() : NULL;

    if (pArea)
    {
        if (pArea->m_pObjectsTree)
        {
            pArea->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
        }

        if (pLightArea)
        {
            // check neighbor sectors and portals if light and object are not in same area
            if (!(pLight->m_Flags & DLF_THIS_AREA_ONLY))
            {
                for (int pp = 0; pp < pArea->m_lstConnections.Count(); pp++)
                {
                    CVisArea* pN = pArea->m_lstConnections[pp];
                    if (pN->m_pObjectsTree)
                    {
                        pN->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
                    }

                    for (int p = 0; p < pN->m_lstConnections.Count(); p++)
                    {
                        CVisArea* pNN = pN->m_lstConnections[p];
                        if (pNN != pLightArea && pNN->m_pObjectsTree)
                        {
                            pNN->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
                        }
                    }
                }
            }
            else if (!pLightArea->IsPortal())
            { // visit also portals
                for (int p = 0; p < pArea->m_lstConnections.Count(); p++)
                {
                    CVisArea* pN = pArea->m_lstConnections[p];
                    if (pN->m_pObjectsTree)
                    {
                        pN->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
                    }
                }
            }
        }
    }
    else
    {
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
        }

        // check also visareas effected by sun
        CVisAreaManager* pVisAreaManager = GetVisAreaManager();
        if (pVisAreaManager)
        {
            {
                PodArray<CVisArea*>& lstAreas = pVisAreaManager->m_lstVisAreas;
                for (int i = 0; i < lstAreas.Count(); i++)
                {
                    if (lstAreas[i]->IsAffectedByOutLights() && lstAreas[i]->m_pObjectsTree)
                    {
                        bool bUnused = false;
                        if (pFr->IntersectAABB(*lstAreas[i]->GetAABBox(), &bUnused))
                        {
                            if (!pShadowHull || IsAABBInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), *lstAreas[i]->GetAABBox()))
                            {
                                lstAreas[i]->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
                            }
                        }
                    }
                }
            }
            {
                PodArray<CVisArea*>& lstAreas = pVisAreaManager->m_lstPortals;
                for (int i = 0; i < lstAreas.Count(); i++)
                {
                    if (lstAreas[i]->IsAffectedByOutLights() && lstAreas[i]->m_pObjectsTree)
                    {
                        bool bUnused = false;
                        if (pFr->IntersectAABB(*lstAreas[i]->GetAABBox(), &bUnused))
                        {
                            if (!pShadowHull || IsAABBInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), *lstAreas[i]->GetAABBox()))
                            {
                                lstAreas[i]->m_pObjectsTree->FillShadowCastersList(false, pLight, pFr, pShadowHull, nRenderNodeFlags, passInfo);
                            }
                        }
                    }
                }
            }
        }
    }

    // add casters with per object shadow map for point lights
    if ((pLight->m_Flags & DLF_SUN) == 0)
    {
        for (uint i = 0; i < Get3DEngine()->m_lstPerObjectShadows.size(); ++i)
        {
            IShadowCaster* pCaster = Get3DEngine()->m_lstPerObjectShadows[i].pCaster;
            assert(pCaster);

            AABB casterBox = pCaster->GetBBoxVirtual();

            if (!IsRenderNodeTypeEnabled(pCaster->GetRenderNodeType()))
            {
                continue;
            }

            bool bObjCompletellyInFrustum = false;
            if (!pFr->IntersectAABB(casterBox, &bObjCompletellyInFrustum))
            {
                continue;
            }

            pFr->m_castersList.Add(pCaster);
        }
    }
}

int CObjManager::MakeStaticShadowCastersList(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int nMaxNodes, const SRenderingPassInfo& passInfo)
{
    int nRemainingNodes = nMaxNodes;

    int nNumTrees = 1;
    if (CVisAreaManager* pVisAreaManager = GetVisAreaManager())
    {
        nNumTrees += pVisAreaManager->m_lstVisAreas.size() + pVisAreaManager->m_lstPortals.size();
    }

    // objects tree first
    int nStartSID = pFrustum->pShadowCacheData->mOctreePath[0];
    if (Get3DEngine()->IsObjectTreeReady())
    {
        Get3DEngine()->GetObjectTree()->GetShadowCastersTimeSliced(pIgnoreNode, pFrustum, renderNodeExcludeFlags, nRemainingNodes, 1, passInfo);
    }

    if (nRemainingNodes <= 0)
    {
        return nRemainingNodes;
    }

    pFrustum->pShadowCacheData->mOctreePath[0]++;

    // Vis Areas
    CVisAreaManager* pVisAreaManager = GetVisAreaManager();
    if (pVisAreaManager)
    {
        PodArray<CVisArea*>* lstAreaTypes[] =
        {
            &pVisAreaManager->m_lstVisAreas,
            &pVisAreaManager->m_lstPortals
        };

        //The minus one is a legacy counting issue. Essentially skipping the octree in the engine.
        nStartSID = max(0, nStartSID - 1);

        const int nNumAreaTypes = sizeof(lstAreaTypes) / sizeof(lstAreaTypes[0]);
        for (int nAreaType = 0; nAreaType < nNumAreaTypes; ++nAreaType)
        {
            PodArray<CVisArea*>& lstAreas = *lstAreaTypes[nAreaType];

            for (int i = nStartSID; i < lstAreas.Count(); i++)
            {
                if (lstAreas[i]->IsAffectedByOutLights() && lstAreas[i]->m_pObjectsTree)
                {
                    if (pFrustum->aabbCasters.IsReset() || Overlap::AABB_AABB(pFrustum->aabbCasters, *lstAreas[i]->GetAABBox()))
                    {
                        lstAreas[i]->m_pObjectsTree->GetShadowCastersTimeSliced(pIgnoreNode, pFrustum, renderNodeExcludeFlags, nRemainingNodes, 0, passInfo);
                    }
                }

                if (nRemainingNodes <= 0)
                {
                    return nRemainingNodes;
                }

                pFrustum->pShadowCacheData->mOctreePath[0]++;
            }

            nStartSID = max(0, nStartSID - lstAreas.Count());
        }
    }


    // if we got here we processed every tree, so reset tree index
    pFrustum->pShadowCacheData->mOctreePath[0] = 0;
    return nRemainingNodes;
}
