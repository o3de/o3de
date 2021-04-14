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

// Description : prepare and add render element into renderer


#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "../RenderDll/Common/Shadow_Renderer.h"
#include "IndexedMesh.h"
#include "VisAreas.h"
#include "GeomQuery.h"
#include "MatMan.h"
#include <CryPath.h>

void CStatObj::Render(const SRendParams& rParams, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE;

    if (m_nFlags & STATIC_OBJECT_HIDDEN)
    {
        return;
    }

#ifndef _RELEASE
    int nMaxDrawCalls = GetCVars()->e_MaxDrawCalls;
    if (nMaxDrawCalls > 0)
    {
        // Don't calculate the number of drawcalls every single time a statobj is rendered.
        // This creates a flickering effect with objects appearing and disappearing indicating that the limit has been reached.
        static int nCurrObjCounter = 0;
        if (((nCurrObjCounter++) & 31) == 1)
        {
            if (GetRenderer()->GetCurrentNumberOfDrawCalls() > nMaxDrawCalls)
            {
                return;
            }
        }
    }
#endif // _RELEASE

    CRenderObject* pObj = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
    FillRenderObject(rParams, rParams.pRenderNode, m_pMaterial, NULL, pObj, passInfo);

    RenderInternal(pObj, rParams.nSubObjHideMask, rParams.lodValue, passInfo, SRendItemSorter(rParams.rendItemSorter), rParams.bForceDrawStatic);
}

void CStatObj::RenderStreamingDebugInfo([[maybe_unused]] CRenderObject* pRenderObject)
{
#ifndef _RELEASE
    //  CStatObj * pStreamable = m_pParentObject ? m_pParentObject : this;

    IStatObj* pStreamable = m_pLod0 ? m_pLod0 : this;

    int nKB = 0;

    if (pStreamable->GetRenderMesh())
    {
        nKB += pStreamable->GetRenderMeshMemoryUsage();
    }

    for (int nLod = 1; pStreamable->GetLods() && nLod < MAX_STATOBJ_LODS_NUM; nLod++)
    {
        IStatObj* pLod = (CStatObj*)pStreamable->GetLods()[nLod];

        if (!pLod)
        {
            continue;
        }

        if (pLod->GetRenderMesh())
        {
            nKB += pLod->GetRenderMeshMemoryUsage();
        }
    }

    nKB >>= 10;

    if (nKB > GetCVars()->e_StreamCgfDebugMinObjSize)
    {
        //      nKB = GetStreamableContentMemoryUsage(true) >> 10;

        const char* pComment = 0;

        pStreamable = pStreamable->GetParentObject() ? pStreamable->GetParentObject() : pStreamable;

        if (!pStreamable->IsUnloadable())
        {
            pComment = "No stream";
        }
        else if (!pStreamable->IsLodsAreLoadedFromSeparateFile() && pStreamable->GetLoadedLodsNum())
        {
            pComment = "Single";
        }
        else if (pStreamable->GetLoadedLodsNum() > 1)
        {
            pComment = "Split";
        }
        else
        {
            pComment = "No LODs";
        }

        int nDiff = SATURATEB(int(float(nKB - GetCVars()->e_StreamCgfDebugMinObjSize) / max((int)1, GetCVars()->e_StreamCgfDebugMinObjSize) * 255));
        DrawBBoxLabeled(AABB(m_vBoxMin, m_vBoxMax), pRenderObject->m_II.m_Matrix, ColorB(nDiff, 255 - nDiff, 0, 255),
            "%.2f mb, %s", 1.f / 1024.f * (float)nKB, pComment);
    }
#endif //_RELEASE
}

//////////////////////////////////////////////////////////////////////
void CStatObj::RenderCoverInfo(CRenderObject* pRenderObject)
{
    for (int i = 0; i < GetSubObjectCount(); ++i)
    {
        const IStatObj::SSubObject* subObject = GetSubObject(i);
        if (subObject->nType != STATIC_SUB_OBJECT_DUMMY)
        {
            continue;
        }
        if (strstr(subObject->name, "$cover") == 0)
        {
            continue;
        }

        Vec3 localBoxMin = -subObject->helperSize * 0.5f;
        Vec3 localBoxMax = subObject->helperSize * 0.5f;

        GetRenderer()->GetIRenderAuxGeom()->DrawAABB(
            AABB(localBoxMin, localBoxMax),
            pRenderObject->m_II.m_Matrix * subObject->localTM,
            true, ColorB(192, 0, 255, 255),
            eBBD_Faceted);
    }
}

//////////////////////////////////////////////////////////////////////
void CStatObj::FillRenderObject(const SRendParams& rParams, IRenderNode* pRenderNode, _smart_ptr<IMaterial> pMaterial,
    SInstancingInfo* pInstInfo, CRenderObject*& pObj, const SRenderingPassInfo& passInfo)
{
    //  FUNCTION_PROFILER_3DENGINE;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Specify transformation
    ////////////////////////////////////////////////////////////////////////////////////////////////////


    assert(pObj);
    if (!pObj)
    {
        return;
    }

    pObj->m_pRenderNode = pRenderNode;
    pObj->m_fSort = rParams.fCustomSortOffset;
    SRenderObjData* pOD = NULL;
    if (rParams.pInstance || rParams.m_pVisArea || pInstInfo || rParams.nVisionParams || rParams.nHUDSilhouettesParams || rParams.nSubObjHideMask)
    {
        pOD = pObj->GetObjData();

        pOD->m_uniqueObjectId = reinterpret_cast<uintptr_t>(rParams.pInstance);
        pOD->m_nHUDSilhouetteParams = rParams.nHUDSilhouettesParams;

        if (rParams.nSubObjHideMask && (m_pMergedRenderMesh != NULL))
        {
            // Only pass SubObject hide mask for merged objects, because they have a correct correlation between Hide Mask and Render Chunks.
            pOD->m_nSubObjHideMask = rParams.nSubObjHideMask;
            pObj->m_ObjFlags |= FOB_MESH_SUBSET_INDICES;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set flags
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    pObj->m_ObjFlags |= rParams.dwFObjFlags;

    if (rParams.nTextureID >= 0)
    {
        pObj->m_nTextureID = rParams.nTextureID;
    }

    assert(rParams.pMatrix);
    {
        pObj->m_II.m_Matrix = *rParams.pMatrix;
    }

    pObj->m_II.m_AmbColor = rParams.AmbientColor;
    pObj->m_nClipVolumeStencilRef = rParams.nClipVolumeStencilRef;
    pObj->m_ObjFlags |= FOB_PARTICLE_SHADOWS;
    pObj->m_fAlpha = rParams.fAlpha;
    pObj->m_DissolveRef = rParams.nDissolveRef;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Process bending
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pRenderNode && pRenderNode->GetRndFlags() & ERF_RECVWIND)
    {
        Get3DEngine()->SetupBending(pObj, pRenderNode, m_fRadiusVert, passInfo, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set render quality
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    pObj->m_nRenderQuality = (uint16)(rParams.fRenderQuality * 65535.0f);
    pObj->m_fDistance = rParams.fDistance;

    {
        //clear, when exchange the state of pLightMapInfo to NULL, the pObj parameters must be update...
        pObj->m_nSort = fastround_positive(rParams.fDistance * 2.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Add render elements
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    if (rParams.pMaterial)
    {
        pMaterial = rParams.pMaterial;
    }

    // prepare multi-layer stuff to render object
    if (!rParams.nMaterialLayersBlend && rParams.nMaterialLayers)
    {
        uint8 nFrozenLayer = (rParams.nMaterialLayers & MTL_LAYER_FROZEN) ? MTL_LAYER_FROZEN_MASK : 0;
        uint8 nWetLayer = (rParams.nMaterialLayers & MTL_LAYER_WET) ? MTL_LAYER_WET_MASK : 0;
        pObj->m_nMaterialLayers = (uint32)(nFrozenLayer << 24) | (nWetLayer << 16);
    }
    else
    {
        pObj->m_nMaterialLayers = rParams.nMaterialLayersBlend;
    }

    if (rParams.nCustomData || rParams.nCustomFlags)
    {
        if (!pOD)
        {
            pOD = pObj->GetObjData();
        }

        pOD->m_nCustomData = rParams.nCustomData;
        pOD->m_nCustomFlags = rParams.nCustomFlags;
    }

    if (rParams.nAfterWater)
    {
        pObj->m_ObjFlags |= FOB_AFTER_WATER;
    }
    else
    {
        pObj->m_ObjFlags &= ~FOB_AFTER_WATER;
    }

    pObj->m_pRenderNode = rParams.pRenderNode;
    pObj->m_pCurrMaterial = pMaterial;
    pObj->m_NoDecalReceiver = rParams.NoDecalReceiver;
    if (Get3DEngine()->IsTessellationAllowed(pObj, passInfo))
    {
        // Allow this RO to be tessellated, however actual tessellation will be applied if enabled in material
        pObj->m_ObjFlags |= FOB_ALLOW_TESSELLATION;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CStatObj::RenderDebugInfo([[maybe_unused]] CRenderObject* pObj, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
#ifndef _RELEASE

    if (!passInfo.IsGeneralPass())
    {
        return false;
    }

    IRenderer* pRend = GetRenderer();
    _smart_ptr<IMaterial> pMaterial = pObj->m_pCurrMaterial;

    IRenderAuxGeom* pAuxGeom = GetRenderer()->GetIRenderAuxGeom();
    if (!pAuxGeom)
    {
        return false;
    }

    Matrix34 tm = pObj->m_II.m_Matrix;

    // Convert "camera space" to "world space"
    if (pObj->m_ObjFlags & FOB_NEAREST)
    {
        tm.AddTranslation(gEnv->pRenderer->GetCamera().GetPosition());
    }

    AABB bbox(m_vBoxMin, m_vBoxMax);

    bool bOnlyBoxes = GetCVars()->e_DebugDraw == -1;

    int e_DebugDraw = GetCVars()->e_DebugDraw;
    string e_DebugDrawFilter = GetCVars()->e_DebugDrawFilter->GetString();
    bool bHasHelperFilter = e_DebugDrawFilter != "";
    bool bFiltered = false;

    if (e_DebugDraw == 1)
    {
        string name;
        if (!m_szGeomName.empty())
        {
            name = m_szGeomName.c_str();
        }
        else
        {
            name = PathUtil::GetFile(m_szFileName.c_str());
        }

        bFiltered = name.find(e_DebugDrawFilter) == string::npos;
    }

    if ((GetCVars()->e_DebugDraw == 1 || bOnlyBoxes) && !bFiltered)
    {
        if (!m_bMerged)
        {
            pAuxGeom->DrawAABB(bbox, tm, false, ColorB(0, 255, 255, 128), eBBD_Faceted);
        }
        else
        {
            pAuxGeom->DrawAABB(bbox, tm, false, ColorB(255, 200, 0, 128), eBBD_Faceted);
        }
    }


    bool bNoText = e_DebugDraw < 0;
    if (e_DebugDraw < 0)
    {
        e_DebugDraw = -e_DebugDraw;
    }



    if (m_nRenderTrisCount > 0 && !bOnlyBoxes && !bFiltered)
    { // cgf's name and tris num
        int nThisLod = 0;
        if (m_pLod0 && m_pLod0->GetLods())
        {
            for (int i = 0; i < MAX_STATOBJ_LODS_NUM; i++)
            {
                if (m_pLod0->GetLods()[i] == this)
                {
                    nThisLod = i;
                    break;
                }
            }
        }

        const int nMaxUsableLod = (m_pLod0) ? m_pLod0->GetMaxUsableLod() : m_nMaxUsableLod;
        const int nRealNumLods = (m_pLod0) ? m_pLod0->GetLoadedLodsNum() : m_nLoadedLodsNum;

        int nNumLods = nRealNumLods;
        if (nNumLods > nMaxUsableLod + 1)
        {
            nNumLods = nMaxUsableLod + 1;
        }

        int nLod = nThisLod;
        if (nLod > nNumLods - 1)
        {
            nLod = nNumLods - 1;
        }

        Vec3 pos = tm.TransformPoint((m_vBoxMin + m_vBoxMax) * 0.5f);
        float color[4] = {1, 1, 1, 1};
        int nMats = m_pRenderMesh ? m_pRenderMesh->GetChunks().size() : 0;
        int nRenderMats = 0;

        if (nMats)
        {
            for (int i = 0; i < nMats; ++i)
            {
                CRenderChunk& rc = m_pRenderMesh->GetChunks()[i];
                if (rc.pRE && rc.nNumIndices && rc.nNumVerts && ((rc.m_nMatFlags & MTL_FLAG_NODRAW) == 0))
                {
                    ++nRenderMats;
                }
            }
        }

        switch (e_DebugDraw)
        {
        case 1:
        {
            const char* shortName = "";
            if (!m_szGeomName.empty())
            {
                shortName = m_szGeomName.c_str();
            }
            else
            {
                shortName = PathUtil::GetFile(m_szFileName.c_str());
            }
            if (nNumLods > 1)
            {
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s\n%d (LOD %d/%d)", shortName, m_nRenderTrisCount, nLod, nNumLods);
            }
            else
            {
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s\n%d", shortName, m_nRenderTrisCount);
            }
        }
        break;

        case 2:
        {
            //////////////////////////////////////////////////////////////////////////
            // Show colored poly count.
            //////////////////////////////////////////////////////////////////////////
            int fMult = 1;
            int nTris = m_nRenderTrisCount;
            ColorB clr = ColorB(0, 0, 0, 255);
            if (nTris >= 20000 * fMult)
            {
                clr = ColorB(255, 0, 0, 255);
            }
            else if (nTris >= 10000 * fMult)
            {
                clr = ColorB(255, 255, 0, 255);
            }
            else if (nTris >= 5000 * fMult)
            {
                clr = ColorB(0, 255, 0, 255);
            }
            else if (nTris >= 2500 * fMult)
            {
                clr = ColorB(0, 255, 255, 255);
            }
            else if (nTris > 1250 * fMult)
            {
                clr = ColorB(0, 0, 255, 255);
            }

            if (pMaterial)
            {
                pMaterial = GetMatMan()->GetDefaultHelperMaterial();
            }
            if (pObj)
            {
                pObj->m_II.m_AmbColor = ColorF(clr.r / 155.0f, clr.g / 155.0f, clr.b / 155.0f, 1);
                pObj->m_nMaterialLayers = 0;
                pObj->m_ObjFlags |= FOB_SELECTED;
            }

            if (!bNoText)
            {
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d", m_nRenderTrisCount);
            }

            return false;
            //////////////////////////////////////////////////////////////////////////
        }
        case 3:
        {
            //////////////////////////////////////////////////////////////////////////
            // Show Lods
            //////////////////////////////////////////////////////////////////////////
            ColorB clr;
            if (nNumLods < 2)
            {
                if (m_nRenderTrisCount <= GetCVars()->e_LodMinTtris  || nRealNumLods > 1)
                {
                    clr = ColorB(50, 50, 50, 255);
                }
                else
                {
                    clr = ColorB(255, 0, 0, 255);
                    float fAngle = gEnv->pTimer->GetFrameStartTime().GetPeriodicFraction(1.0f) * gf_PI2;
                    clr.g = 127 + (int)(sinf(fAngle) * 120);      // flashing color
                }
            }
            else
            {
                if (nLod == 0)
                {
                    clr = ColorB(255, 0, 0, 255);
                }
                else if (nLod == 1)
                {
                    clr = ColorB(0, 255, 0, 255);
                }
                else if (nLod == 2)
                {
                    clr = ColorB(0, 0, 255, 255);
                }
                else if (nLod == 3)
                {
                    clr = ColorB(0, 255, 255, 255);
                }
                else if (nLod == 4)
                {
                    clr = ColorB(255, 255, 0, 255);
                }
                else if (nLod == 5)
                {
                    clr = ColorB(255, 0, 255, 255);
                }
                else
                {
                    clr = ColorB(255, 255, 255, 255);
                }
            }

            if (pMaterial)
            {
                pMaterial = GetMatMan()->GetDefaultHelperMaterial();
            }
            if (pObj)
            {
                pObj->m_II.m_AmbColor = ColorF(clr.r / 180.0f, clr.g / 180.0f, clr.b / 180.0f, 1);
                pObj->m_nMaterialLayers = 0;
                pObj->m_ObjFlags |= FOB_SELECTED;
            }

            // Don't skip objects with single lod (as they should flash)
            if (pObj && !bNoText)
            {
                const int nLod0 = nNumLods > 1 ? GetMinUsableLod() : 0;     // Always 0 if one lod
                const int maxLod = nNumLods > 1 ? GetMaxUsableLod() : 0;    // Always 0 if one lod

                clr.toFloat4(color);    // Actually use the colours calculated already to indicate lod

                const bool bRenderNodeValid(pObj && pObj->m_pRenderNode && ((UINT_PTR)(void*)(pObj->m_pRenderNode) > 0));
                IRenderNode* pRN = (IRenderNode*)pObj->m_pRenderNode;
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d [%d;%d] (%d/%.1f)",
                    nLod, nLod0, maxLod,
                    bRenderNodeValid ? pRN->GetLodRatio() : -1, pObj->m_fDistance);
            }

            return false;
            //////////////////////////////////////////////////////////////////////////
        }
        case 4:
        {
            // Show texture usage.
            if (m_pRenderMesh)
            {
                int nTexMemUsage = m_pRenderMesh->GetTextureMemoryUsage(pMaterial);
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d", nTexMemUsage / 1024);      // in KByte
            }
        }
        break;

        case 5:
        {
            //////////////////////////////////////////////////////////////////////////
            // Show Num Render materials.
            //////////////////////////////////////////////////////////////////////////
            ColorB clr(0, 0, 0, 0);
            if (nRenderMats == 1)
            {
                clr = ColorB(0, 0, 255, 255);
            }
            else if (nRenderMats == 2)
            {
                clr = ColorB(0, 255, 255, 255);
            }
            else if (nRenderMats == 3)
            {
                clr = ColorB(0, 255, 0, 255);
            }
            else if (nRenderMats == 4)
            {
                clr = ColorB(255, 0, 255, 255);
            }
            else if (nRenderMats == 5)
            {
                clr = ColorB(255, 255, 0, 255);
            }
            else if (nRenderMats >= 6)
            {
                clr = ColorB(255, 0, 0, 255);
            }
            else if (nRenderMats >= 11)
            {
                clr = ColorB(255, 255, 255, 255);
            }

            if (pMaterial)
            {
                pMaterial = GetMatMan()->GetDefaultHelperMaterial();
            }
            if (pObj)
            {
                pObj->m_II.m_AmbColor = ColorF(clr.r / 155.0f, clr.g / 155.0f, clr.b / 155.0f, 1);
                pObj->m_nMaterialLayers = 0;
                pObj->m_ObjFlags |= FOB_SELECTED;
            }

            if (!bNoText)
            {
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d", nRenderMats);
            }
        }
        break;

        case 6:
        {
            if (pMaterial)
            {
                pMaterial = GetMatMan()->GetDefaultHelperMaterial();
            }

            ColorF col(1, 1, 1, 1);
            if (pObj)
            {
                pObj->m_nMaterialLayers = 0;
                col = pObj->m_II.m_AmbColor;
            }

            pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d,%d,%d,%d", (int)(col.r * 255.0f), (int)(col.g * 255.0f), (int)(col.b * 255.0f), (int)(col.a * 255.0f));
        }
        break;

        case 7:
            if (m_pRenderMesh)
            {
                int nTexMemUsage = m_pRenderMesh->GetTextureMemoryUsage(pMaterial);
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d,%d,%d", m_nRenderTrisCount, nRenderMats, nTexMemUsage / 1024);
            }
            break;
        case 13:
        {
#ifdef SUPPORT_TERRAIN_AO_PRE_COMPUTATIONS
            float fOcclusion = GetOcclusionAmount();
            pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%.2f", fOcclusion);
#endif
        }
        break;

        case 16:
        {
            // Draw stats for object selected by debug gun
            if (GetRenderer()->IsDebugRenderNode((IRenderNode*)pObj->m_pRenderNode))
            {
                const char* shortName = PathUtil::GetFile(m_szFileName.c_str());

                int texMemUsage = 0;

                if (m_pRenderMesh)
                {
                    texMemUsage = m_pRenderMesh->GetTextureMemoryUsage(pMaterial);
                }

                pAuxGeom->DrawAABB(bbox, tm, false, ColorB(0, 255, 255, 128), eBBD_Faceted);

                float yellow[4] = {1.f, 1.f, 0.f, 1.f};

                const float yOffset = 165.f;
                const float xOffset = 970.f;

                if (m_pParentObject == NULL)
                {
                    pRend->Draw2dLabel(xOffset, 40.f, 1.5f, yellow, false, "%s", shortName);

                    pRend->Draw2dLabel(xOffset, yOffset, 1.5f, color, false,
                        //"Mesh: %s\n"
                        "LOD: %d/%d\n"
                        "Num Instances: %d\n"
                        "Num Tris: %d\n"
                        "Tex Mem usage: %.2f kb\n"
                        "Mesh Mem usage: %.2f kb\n"
                        "Num Materials: %d\n"
                        "Mesh Type: %s\n",
                        //shortName,
                        nLod, nNumLods,
                        m_nUsers,
                        m_nRenderTrisCount,
                        texMemUsage / 1024.f,
                        m_nRenderMeshMemoryUsage / 1024.f,
                        nRenderMats,
                        m_pRenderMesh->GetTypeName());
                }
                else
                {
                    for (int i = 0; i < m_pParentObject->SubObjectCount(); i++)
                    {
                        //find subobject position
                        if (m_pParentObject->SubObject(i).pStatObj == this)
                        {
                            //only render the header once
                            if (i == 0)
                            {
                                pRend->Draw2dLabel(600.f, 40.f, 2.f, yellow, false, "Debug Gun: %s", shortName);
                            }
                            float y = yOffset + ((i % 4) * 150.f);
                            float x = xOffset - (floor(i / 4.f) * 200.f);

                            pRend->Draw2dLabel(x, y, 1.5f, color, false,
                                "Sub Mesh: %s\n"
                                "LOD: %d/%d\n"
                                "Num Instances: %d\n"
                                "Num Tris: %d\n"
                                "Tex Mem usage: %.2f kb\n"
                                "Mesh Mem usage: %.2f kb\n"
                                "Num Materials: %d\n"
                                "Mesh Type: %s\n",
                                m_szGeomName.c_str() ? m_szGeomName.c_str() : "UNKNOWN",
                                nLod, nNumLods,
                                m_nUsers,
                                m_nRenderTrisCount,
                                texMemUsage / 1024.f,
                                m_nRenderMeshMemoryUsage / 1024.f,
                                nRenderMats,
                                m_pRenderMesh->GetTypeName());

                            break;
                        }
                    }
                }
            }
        }
        break;

        case 19:    // Displays the triangle count of physic proxies.
            if (!bNoText)
            {
                int nPhysTrisCount = 0;
                for (int j = 0; j < MAX_PHYS_GEOMS_TYPES; ++j)
                {
                    if (GetPhysGeom(j))
                    {
                        nPhysTrisCount += GetPhysGeom(j)->pGeom->GetPrimitiveCount();
                    }
                }

                if (nPhysTrisCount == 0)
                {
                    color[3] = 0.1f;
                }

                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%d", nPhysTrisCount);
            }
            return false;

        case 22:
        {
            // Show texture usage.
            if (m_pRenderMesh)
            {
                pRend->DrawLabelEx(pos, 1.3f, color, true, true, "[LOD %d: %d]", nLod, m_pRenderMesh->GetVerticesCount());
            }
        }
        break;
        case 23:
        {
            if (pObj && pObj->m_pRenderNode)
            {
                IRenderNode* pRenderNode = (IRenderNode*)pObj->m_pRenderNode;
                const bool bCastsShadow = (pRenderNode->GetRndFlags() & ERF_CASTSHADOWMAPS) != 0;
                ColorF clr = bCastsShadow ? ColorF(1.f, 0.f, 0.f, 1.0f) : ColorF(0.f, 1.f, 0.f, 1.f);

                int nIndices = 0;
                int nIndicesNoShadow = 0;

                // figure out how many primitives actually cast shadows
                if (pMaterial && bCastsShadow)
                {
                    for (int i = 0; i < nMats; ++i)
                    {
                        CRenderChunk& rc = m_pRenderMesh->GetChunks()[i];
                        if (rc.pRE && rc.nNumIndices && rc.nNumVerts && ((rc.m_nMatFlags & MTL_FLAG_NODRAW) == 0))
                        {
                            SShaderItem& ShaderItem         = pMaterial->GetShaderItem(rc.m_nMatID);
                            IRenderShaderResources* pR  = ShaderItem.m_pShaderResources;

                            if (pR && (pR->GetResFlags() & MTL_FLAG_NOSHADOW))
                            {
                                nIndicesNoShadow += rc.nNumIndices;
                            }

                            nIndices += rc.nNumIndices;
                        }
                    }

                    Vec3 red, green;
                    ColorF(1.f, 0.f, 0.f, 1.0f).toHSV(red.x, red.y, red.z);
                    ColorF(0.f, 1.f, 0.f, 1.0f).toHSV(green.x, green.y, green.z);

                    Vec3 c = Vec3::CreateLerp(red, green, (float)nIndicesNoShadow / max(nIndices, 1));
                    clr.fromHSV(c.x, c.y, c.z);

                    pMaterial = GetMatMan()->GetDefaultHelperMaterial();
                }

                pObj->m_II.m_AmbColor = clr;
                pObj->m_nMaterialLayers = 0;
                pObj->m_ObjFlags |= FOB_SELECTED;
            }
            return false;
        }
        case 24:
        case 25:
        {
            // display a label for this render node if the triangle count equals or exceeds 
            // e_DebugDrawLodMinTriangles and the object has no lods, or has too few lods
            int minTriangleCount = GetCVars()->e_DebugDrawLodMinTriangles;
            if (m_nLoadedTrisCount >= minTriangleCount)
            {
                IRenderNode* pRN = (IRenderNode*)pObj->m_pRenderNode;
                const char* shortName = "";

                if (!m_szGeomName.empty())
                {
                    shortName = m_szGeomName.c_str();
                }
                else
                {
                    shortName = PathUtil::GetFile(m_szFileName.c_str());
                }

                if (nNumLods == 1)
                {
                    color[1] = color[2] = 0.0f;

                    IRenderer::RNDrawcallsMapNode& drawCallsPerNode = pRend->GetDrawCallsInfoPerNodePreviousFrame();
                    auto iter = drawCallsPerNode.find(pRN);
                    if (iter != drawCallsPerNode.end())
                    {
                        pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s (%d)\n%d/%d/%d/%d/%d", shortName, m_nLoadedTrisCount, iter->second.nZpass, iter->second.nGeneral, iter->second.nTransparent, iter->second.nShadows, iter->second.nMisc);
                    }
                    else
                    {
                        pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s (%d)", shortName, m_nLoadedTrisCount);
                    }
                }
                else if (e_DebugDraw == 25 && nNumLods < MAX_STATOBJ_LODS_NUM)   // 25 adds in drawing of objects that should be at a lower lod than exists
                {
                    float lodDistance = sqrt(m_fGeometricMeanFaceArea);
                    float nextLodDistance = lodDistance * (nNumLods) / (pRN->GetLodRatioNormalized() * gEnv->p3DEngine->GetFrameLodInfo().fTargetSize);
                    if (pObj->m_fDistance > nextLodDistance)
                    {
                        color[0] = color[1] = 0.0f;
                        pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s (%d)", shortName, m_nLoadedTrisCount);
                    }
                }
            }
        }
        break;
        }
    }

    if (GetCVars()->e_DebugDraw == 15 && !bOnlyBoxes)
    {
        // helpers
        for (int i = 0; i < (int)m_subObjects.size(); i++)
        {
            SSubObject* pSubObject = &(m_subObjects[i]);
            if (pSubObject->nType == STATIC_SUB_OBJECT_MESH && pSubObject->pStatObj)
            {
                continue;
            }

            if (bHasHelperFilter)
            {
                if (pSubObject->name.find(e_DebugDrawFilter) == string::npos)
                {
                    continue;
                }
            }

            // make object matrix
            Matrix34 tMat = tm * pSubObject->tm;
            Vec3 pos = tMat.GetTranslation();

            // draw axes
            float s = 0.02f;
            ColorB col(0, 255, 255, 255);
            pAuxGeom->DrawAABB(AABB(Vec3(-s, -s, -s), Vec3(s, s, s)), tMat, false, col, eBBD_Faceted);
            pAuxGeom->DrawLine(pos + s * tMat.GetColumn1(), col, pos + 3.f * s * tMat.GetColumn1(), col);

            // text
            float color[4] = {0, 1, 1, 1};
            pRend->DrawLabelEx(pos, 1.3f, color, true, true, "%s", pSubObject->name.c_str());
        }
    }


    if (Get3DEngine()->IsDebugDrawListEnabled())
    {
        I3DEngine::SObjectInfoToAddToDebugDrawList objectInfo;
        if (pObj->m_pRenderNode)
        {
            objectInfo.pName = ((IRenderNode*)pObj->m_pRenderNode)->GetName();
            objectInfo.pClassName = ((IRenderNode*)pObj->m_pRenderNode)->GetEntityClassName();
        }
        else
        {
            objectInfo.pName = "";
            objectInfo.pClassName = "";
        }
        objectInfo.pFileName = m_szFileName.c_str();
        if (m_pRenderMesh && pObj->m_pCurrMaterial)
        {
            objectInfo.texMemory = m_pRenderMesh->GetTextureMemoryUsage(pObj->m_pCurrMaterial);
        }
        else
        {
            objectInfo.texMemory = 0;
        }
        objectInfo.numTris = m_nRenderTrisCount;
        objectInfo.numVerts = m_nLoadedVertexCount;
        objectInfo.meshMemory = m_nRenderMeshMemoryUsage;
        objectInfo.pMat = &tm;
        objectInfo.pBox = &bbox;
        objectInfo.type = I3DEngine::DLOT_STATOBJ;
        objectInfo.pRenderNode = (IRenderNode*)(pObj->m_pRenderNode);
        Get3DEngine()->AddObjToDebugDrawList(objectInfo);
    }



#endif //_RELEASE
    return false;
}

//
// StatObj functions.
//

float CStatObj::GetExtent(EGeomForm eForm)
{
    int nSubCount = m_subObjects.size();
    if (!nSubCount)
    {
        return m_pRenderMesh ? m_pRenderMesh->GetExtent(eForm) : 0.f;
    }

    CGeomExtent& ext = m_Extents.Make(eForm);
    if (!ext)
    {
        // Create parts for main and sub-objects.
        ext.ReserveParts(1 + nSubCount);

        ext.AddPart(m_pRenderMesh ? m_pRenderMesh->GetExtent(eForm) : 0.f);

        // Evaluate sub-objects.
        for (int i = 0; i < nSubCount; i++)
        {
            IStatObj::SSubObject* pSub = &m_subObjects[i];
            if (pSub->nType == STATIC_SUB_OBJECT_MESH && pSub->pStatObj)
            {
                float fExt = pSub->pStatObj->GetExtent(eForm);

                if (eForm == GeomForm_Edges)
                {
                    fExt *= powf(pSub->tm.Determinant(), 0.333f);
                }
                else if (eForm == GeomForm_Surface)
                {
                    fExt *= powf(pSub->tm.Determinant(), 0.667f);
                }
                else if (eForm == GeomForm_Volume)
                {
                    fExt *= pSub->tm.Determinant();
                }

                ext.AddPart(fExt);
            }
            else
            {
                ext.AddPart(0.f);
            }
        }
    }
    return ext.TotalExtent();
}

void CStatObj::GetRandomPos(PosNorm& ran, EGeomForm eForm) const
{
    if (!m_subObjects.empty())
    {
        CGeomExtent const& ext = m_Extents[eForm];
        int iSubObj = ext.RandomPart();
        if (iSubObj-- > 0)
        {
            IStatObj::SSubObject const* pSub = &m_subObjects[iSubObj];
            assert(pSub && pSub->pStatObj);
            pSub->pStatObj->GetRandomPos(ran, eForm);
            ran <<= pSub->tm;
            return;
        }
    }
    if (m_pRenderMesh)
    {
        m_pRenderMesh->GetRandomPos(ran, eForm);
    }
    else
    {
        ran.zero();
    }
}

void CStatObj::ComputeGeometricMean(SMeshLodInfo& lodInfo)
{
    lodInfo.Clear();

    lodInfo.fGeometricMean = m_fGeometricMeanFaceArea;
    lodInfo.nFaceCount = m_nRenderTrisCount;

    if (GetFlags() & STATIC_OBJECT_COMPOUND)
    {
        for (uint i = 0; i < m_subObjects.size(); ++i)
        {
            if (m_subObjects[i].nType == STATIC_SUB_OBJECT_MESH && m_subObjects[i].bShadowProxy == false && m_subObjects[i].pStatObj != NULL)
            {
                SMeshLodInfo subLodInfo;
                static_cast<CStatObj*>(m_subObjects[i].pStatObj)->ComputeGeometricMean(subLodInfo);

                lodInfo.Merge(subLodInfo);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CStatObj::DebugDraw(const SGeometryDebugDrawInfo& info, float fExtrdueScale)
{
    if (m_nFlags & STATIC_OBJECT_COMPOUND && !m_bMerged)
    {
        // Draw sub objects.
        for (int i = 0; i < (int)m_subObjects.size(); i++)
        {
            if (!m_subObjects[i].pStatObj || m_subObjects[i].bHidden || m_subObjects[i].nType != STATIC_SUB_OBJECT_MESH)
            {
                continue;
            }

            SGeometryDebugDrawInfo subInfo = info;
            subInfo.tm = info.tm * m_subObjects[i].localTM;
            m_subObjects[i].pStatObj->DebugDraw(subInfo, fExtrdueScale);
        }
    }
    else if (m_pRenderMesh)
    {
        m_pRenderMesh->DebugDraw(info, ~0, fExtrdueScale);
    }
    else
    {
        // No RenderMesh in here, so probably no geometry in highest LOD, find it in lower LODs
        if (m_pLODs)
        {
            assert(m_nMaxUsableLod < MAX_STATOBJ_LODS_NUM);
            for (int nLod = 0; nLod <= (int)m_nMaxUsableLod; nLod++)
            {
                if (m_pLODs[nLod] && m_pLODs[nLod]->m_pRenderMesh)
                {
                    m_pLODs[nLod]->m_pRenderMesh->DebugDraw(info, ~0, fExtrdueScale);
                    break;
                }
            }
        }
    }
}

