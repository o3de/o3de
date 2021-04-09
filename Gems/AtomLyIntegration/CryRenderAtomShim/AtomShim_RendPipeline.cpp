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

// Description : NULL device specific implementation using shaders pipeline.


#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "Common/RenderView.h"
#include "RenderBus.h"

//============================================================================================
// Init Shaders rendering

void CAtomShimRenderer::EF_Init()
{
    m_RP.m_MaxVerts = 600;
    m_RP.m_MaxTris = 300;

    //==================================================
    // Init RenderObjects
    {
        m_RP.m_nNumObjectsInPool = 384; // magic number set by Cry.  The regular pipe uses a constant set to 1024

        if (m_RP.m_ObjectsPool != nullptr)
        {
            for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
            {
                CRenderObject* pRendObj = &m_RP.m_ObjectsPool[j];
                pRendObj->~CRenderObject();
            }
            CryModuleMemalignFree(m_RP.m_ObjectsPool);
        }

        // we use a plain allocation and placement new here to garantee the alignment, when using array new, the compiler can store it's size and break the alignment
        m_RP.m_ObjectsPool = (CRenderObject*)CryModuleMemalign(sizeof(CRenderObject) * (m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT), 16);
        for (int j = 0; j < (int)(m_RP.m_nNumObjectsInPool * RT_COMMAND_BUF_COUNT); j++)
        {
            new(&m_RP.m_ObjectsPool[j])CRenderObject();
        }


        CRenderObject** arrPrefill = (CRenderObject**)(alloca(m_RP.m_nNumObjectsInPool * sizeof(CRenderObject*)));
        for (int j = 0; j < RT_COMMAND_BUF_COUNT; j++)
        {
            for (int k = 0; k < m_RP.m_nNumObjectsInPool; ++k)
            {
                arrPrefill[k] = &m_RP.m_ObjectsPool[j * m_RP.m_nNumObjectsInPool + k];
            }

            m_RP.m_TempObjects[j].PrefillContainer(arrPrefill, m_RP.m_nNumObjectsInPool);
            m_RP.m_TempObjects[j].resize(0);
        }
    }
    // Init identity RenderObject
    SAFE_DELETE(m_RP.m_pIdendityRenderObject);
    m_RP.m_pIdendityRenderObject = aznew CRenderObject();
    m_RP.m_pIdendityRenderObject->Init();
    m_RP.m_pIdendityRenderObject->m_II.m_AmbColor = Col_White;
    m_RP.m_pIdendityRenderObject->m_II.m_Matrix.SetIdentity();
    m_RP.m_pIdendityRenderObject->m_RState = 0;
    m_RP.m_pIdendityRenderObject->m_ObjFlags |= FOB_RENDERER_IDENDITY_OBJECT;

}

void CAtomShimRenderer::FX_SetClipPlane ([[maybe_unused]] bool bEnable, [[maybe_unused]] float* pPlane, [[maybe_unused]] bool bRefract)
{
}

void CAtomShimRenderer::FX_PipelineShutdown([[maybe_unused]] bool bFastShutdown)
{
    uint32 i, j;

    for (int n = 0; n < 2; n++)
    {
        for (j = 0; j < 2; j++)
        {
            for (i = 0; i < CREClientPoly::m_PolysStorage[n][j].Num(); i++)
            {
                CREClientPoly::m_PolysStorage[n][j][i]->Release(false);
            }
            CREClientPoly::m_PolysStorage[n][j].Free();
        }
    }
}

void CAtomShimRenderer::EF_Release([[maybe_unused]] int nFlags)
{
}

//==========================================================================

void CAtomShimRenderer::FX_SetState(int st, int AlphaRef, [[maybe_unused]] int RestoreState)
{
    m_RP.m_CurState = st;
    m_RP.m_CurAlphaRef = AlphaRef;
}
void CRenderer::FX_SetStencilState([[maybe_unused]] int st, [[maybe_unused]] uint32 nStencRef, [[maybe_unused]] uint32 nStencMask, [[maybe_unused]] uint32 nStencWriteMask, [[maybe_unused]] bool bForceFullReadMask)
{
}

//=================================================================================

// Initialize of the new shader pipeline (only 2d)
void CRenderer::FX_Start([[maybe_unused]] CShader* ef, [[maybe_unused]] int nTech, [[maybe_unused]] CShaderResources* Res, [[maybe_unused]] IRenderElement* re)
{
    m_RP.m_Frame++;
}

void CRenderer::FX_CheckOverflow([[maybe_unused]] int nVerts, [[maybe_unused]] int nInds, [[maybe_unused]] IRenderElement* re, [[maybe_unused]] int* nNewVerts, [[maybe_unused]] int* nNewInds)
{
}

uint32 CRenderer::EF_GetDeferredLightsNum([[maybe_unused]] const eDeferredLightType eLightType)
{
    return 0;
}

int CRenderer::EF_AddDeferredLight([[maybe_unused]] const CDLight& pLight, float, [[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] const SRendItemSorter& rendItemSorter)
{
    return 0;
}

void CRenderer::EF_ClearDeferredLightsList()
{
}

void CRenderer::EF_ReleaseDeferredData()
{
}

uint8 CRenderer::EF_AddDeferredClipVolume([[maybe_unused]] const IClipVolume* pClipVolume)
{
    return 0;
}


bool CRenderer::EF_SetDeferredClipVolumeBlendData([[maybe_unused]] const IClipVolume* pClipVolume, [[maybe_unused]] const SClipVolumeBlendInfo& blendInfo)
{
    return false;
}

void CRenderer::EF_ClearDeferredClipVolumesList()
{
}

//========================================================================================

void CAtomShimRenderer::EF_EndEf3D([[maybe_unused]] const int nFlags, [[maybe_unused]] const int nPrecacheUpdateId, [[maybe_unused]] const int nNearPrecacheUpdateId, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
    //m_RP.m_TI[m_RP.m_nFillThreadID].m_RealTime = iTimer->GetCurrTime();
    EF_RemovePolysFromScene();

    // Only render the UI Canvas and the Console on the main window
    // If we're not in the editor, don't bother to check viewport.
    if (!gEnv->IsEditor() || m_currContext->m_isMainViewport)
    {
        EBUS_EVENT(AZ::RenderNotificationsBus, OnScene3DEnd);
    }

    int nThreadID = m_pRT->GetThreadList();
    SRendItem::m_RecurseLevel[nThreadID]--;
}

//double timeFtoI, timeFtoL, timeQRound;
//int sSome;
void CAtomShimRenderer::EF_EndEf2D([[maybe_unused]] const bool bSort)
{
}

void CRenderView::PrepareForRendering() {}

void CRenderView::PrepareForWriting() {}

void CRenderView::ClearRenderItems() {}

void CRenderView::FreeRenderItems() {}

CRenderView::CRenderView() {}

CRenderView::~CRenderView() {}

