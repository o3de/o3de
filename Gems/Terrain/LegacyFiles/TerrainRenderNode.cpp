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

#include "StdAfx.h"

#ifdef LY_TERRAIN_RUNTIME

#include "CRETerrain.h"
#include "TerrainRenderNode.h"
#include "ObjMan.h"
#include "VisAreas.h"

namespace Terrain
{
    TerrainRenderNode::TerrainRenderNode(AZStd::string_view terrainSystemMaterialName)
    {
        m_terrainRE = static_cast<::Terrain::CRETerrain*>(GetISystem()->GetIRenderer()->EF_CreateRE(eDATA_TerrainSystem));
        m_dwRndFlags = ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS | ERF_RAIN_OCCLUDER | ERF_RENDER_ALWAYS | ERF_NO_PHYSICS;
        GetISystem()->GetI3DEngine()->RegisterEntity(this);

        // Set this node to have the terrain system material.
        _smart_ptr<IMaterial> pMaterial = GetISystem()->GetI3DEngine()->GetMaterialManager()->LoadMaterial(terrainSystemMaterialName.data(), false);
        SetMaterial(pMaterial);

        // We can't call GetISystem() from the render thread, so save off pointers to
        // the various systems we'll need to call when rendering.
        m_console = GetISystem()->GetIConsole();
        m_renderer = GetISystem()->GetIRenderer();
        m_3dengine = GetISystem()->GetI3DEngine();
    }

    TerrainRenderNode::~TerrainRenderNode()
    {
        m_3dengine->FreeRenderNodeState(this);
        m_terrainRE->Release();
    }


    void TerrainRenderNode::GetMemoryUsage(ICrySizer* pSizer) const
    {
        // todo, workout what this should report
        pSizer->AddObject(this, sizeof(TerrainRenderNode));
        pSizer->AddObject(m_terrainRE);
    }


    void TerrainRenderNode::SetupRenderObject(CRenderObject* pObj, const SRenderingPassInfo& passInfo)
    {
        Vec3 vOrigin(0, 0, 0);

        pObj->m_pRenderNode = this;
        pObj->m_II.m_Matrix.SetIdentity();
        pObj->m_II.m_Matrix.SetTranslation(vOrigin);
        pObj->m_fAlpha = 1.f;
        pObj->m_nSort = fastround_positive(pObj->m_fDistance * 2.0f);

        IMaterial* pMat = pObj->m_pCurrMaterial = GetMaterial();
        pObj->m_ObjFlags &= ~FOB_DYNAMIC_OBJECT;

        if (uint8 nMaterialLayers = IRenderNode::GetMaterialLayers())
        {
            uint8 nFrozenLayer = (nMaterialLayers & MTL_LAYER_FROZEN) ? MTL_LAYER_FROZEN_MASK : 0;
            uint8 nWetLayer = (nMaterialLayers & MTL_LAYER_WET) ? MTL_LAYER_WET_MASK : 0;
            pObj->m_nMaterialLayers = (uint32)(nFrozenLayer << 24) | (nWetLayer << 16);
        }
        else
        {
            pObj->m_nMaterialLayers = 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    void TerrainRenderNode::Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo)
    {
        // Collision proxy is visible in Editor while in editing mode.
        if (m_dwRndFlags & ERF_COLLISION_PROXY)
        {
            if (!gEnv->IsEditor() || !gEnv->IsEditing())
            {
                if (m_console->GetCVar("e_DebugDraw") == 0)
                {
                    return;
                }
            }
        }

        if (m_dwRndFlags & ERF_HIDDEN)
        {
            return;
        }

        if (m_material)
        {
            SRendItemSorter rendItemSorter(EntDrawParams.rendItemSorter);
            CRenderObject* pObj = m_renderer->EF_GetObject_Temp(passInfo.ThreadID());
            SShaderItem& shaderItem(m_material->GetShaderItem(0));

            SetupRenderObject(pObj, passInfo);

            m_terrainRE->mfPrepare(false);
            m_terrainRE->mfDraw(nullptr, nullptr);

            m_renderer->EF_AddEf(m_terrainRE, shaderItem, pObj, passInfo, EFSLIST_TERRAINLAYER, 1, rendItemSorter);

            // NEW-TERRAIN LY-102946:  This can potentially crash if e_BBoxes is enabled while activating / deactivating the
            // TerrainWorld component, since the "this" pointer can get cleaned up at an inappropriate time.
            if (passInfo.IsGeneralPass() && m_console->GetCVar("e_BBoxes"))
            {
                m_3dengine->GetObjManager()->RenderObjectDebugInfo((IRenderNode*)this, pObj->m_fDistance, passInfo);
            }
        }
    }
}

#endif
