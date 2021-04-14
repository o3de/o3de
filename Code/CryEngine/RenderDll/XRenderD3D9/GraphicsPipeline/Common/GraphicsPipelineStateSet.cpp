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

#include "RenderDll_precompiled.h"
#include "GraphicsPipelineStateSet.h"
#include "GraphicsPipelinePass.h"
#include "DriverD3D.h"
#include <AzCore/std/hash.h>

SGraphicsPipelineStateDescription::SGraphicsPipelineStateDescription(
    CRenderObject* pObj,
    const SShaderItem& _shaderItem,
    EShaderTechniqueID _technique,
    AZ::Vertex::Format _vertexFormat,
    uint32 _streamMask,
    int _primitiveType)
{
    shaderItem = _shaderItem;
    technique = _technique;
    objectFlags = pObj->m_ObjFlags;
    objectFlags_MDV = pObj->m_nMDV;
    objectRuntimeMask = pObj->m_nRTMask;
    vertexFormat = _vertexFormat;
    streamMask = _streamMask;
    primitiveType = _primitiveType;

    AZ_PUSH_DISABLE_WARNING(, "-Wconstant-logical-operand")
    if ((pObj->m_ObjFlags & FOB_SKINNED) && CRenderer::CV_r_usehwskinning && !CRenderer::CV_r_character_nodeform)
    AZ_POP_DISABLE_WARNING
    {
        SSkinningData* pSkinningData = NULL;
        SRenderObjData* pOD = pObj->GetObjData();
        if (pOD && pOD->m_pSkinningData)
        {
            pSkinningData = pOD->m_pSkinningData;
            if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_Matrix)
            {
                objectRuntimeMask |= (g_HWSR_MaskBit[HWSR_SKINNING_MATRIX]);
            }
            else if (pSkinningData->nHWSkinningFlags & eHWS_Skinning_DQ_Linear)
            {
                objectRuntimeMask |= (g_HWSR_MaskBit[HWSR_SKINNING_DQ_LINEAR]);
            }
            else
            {
                objectRuntimeMask |= (g_HWSR_MaskBit[HWSR_SKINNING_DUAL_QUAT]);
            }
        }
    }
}

CDeviceGraphicsPSOPtr CGraphicsPipelineStateLocalCache::FindState(uint64 stateHashKey) const
{
    for (auto const& state : m_states)
    {
        if (state.stateHashKey == stateHashKey)
        {
            return state.m_pipelineState;
        }
    }
    return nullptr;
}

uint64 CGraphicsPipelineStateLocalCache::GetHashKey(const SGraphicsPipelineStateDescription& desc) const
{
    AZStd::hash<const SGraphicsPipelineStateDescription*> hasher;
    uint64 key = hasher(&desc);
    return key;
}

const DevicePipelineStatesArray* CGraphicsPipelineStateLocalCache::Find(const SGraphicsPipelineStateDescription& desc) const
{
    uint64 key = GetHashKey(desc);
    for (auto const& state : m_states)
    {
        if (state.stateHashKey == key && state.description == desc)
        {
            return &state.m_pipelineStates;
        }
    }
    return nullptr;
}

void CGraphicsPipelineStateLocalCache::Put(const SGraphicsPipelineStateDescription& desc, const DevicePipelineStatesArray& states)
{
    // Cache this state locally.
    CachedState cache;
    cache.stateHashKey = GetHashKey(desc);
    cache.description = desc;
    cache.m_pipelineStates = states;
    m_states.push_back(cache);
}
