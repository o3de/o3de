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
#pragma once

#include "DeviceManager/DeviceWrapper12.h"

class GraphicsPipelinePass;

struct SGraphicsPipelineStateDescription
{
    SShaderItem              shaderItem;
    EShaderTechniqueID       technique;
    uint64                   objectFlags;
    uint64                   objectRuntimeMask;
    uint32                   objectFlags_MDV;
    AZ::Vertex::Format       vertexFormat;
    uint32                   streamMask;
    int                      primitiveType;

    SGraphicsPipelineStateDescription()
        : technique(TTYPE_Z)
        , objectFlags(0)
        , objectFlags_MDV(0)
        , objectRuntimeMask(0)
        , vertexFormat(eVF_Unknown)
        , streamMask(0)
        , primitiveType(0)
    {};
    SGraphicsPipelineStateDescription(CRenderObject* pObj, const SShaderItem& shaderItem, EShaderTechniqueID technique, AZ::Vertex::Format vertexFormat, uint32 streamMask, int primitiveType);

    bool operator==(const SGraphicsPipelineStateDescription& other) const
    {
        return 0 == memcmp(this, &other, sizeof(*this));
    }
};

// Array of pass id and PipelineState
typedef std::array<CDeviceGraphicsPSOPtr, 4> DevicePipelineStatesArray;

// Set of precomputed Pipeline States
class CGraphicsPipelineStateLocalCache
{
public:
    const DevicePipelineStatesArray* Find(const SGraphicsPipelineStateDescription& desc) const;
    void Put(const SGraphicsPipelineStateDescription& desc, const DevicePipelineStatesArray& states);

private:
    CDeviceGraphicsPSOPtr FindState(uint64 stateHashKey) const;
    void StoreState(uint64 stateHashKey, CDeviceGraphicsPSOPtr pso) const;
    uint64 GetHashKey(const SGraphicsPipelineStateDescription& desc) const;

private:
    struct CachedState
    {
        uint64 stateHashKey;
        SGraphicsPipelineStateDescription description;
        CDeviceGraphicsPSOPtr m_pipelineState;
        DevicePipelineStatesArray m_pipelineStates;
    };
    std::vector<CachedState> m_states;
};
typedef std::shared_ptr<CGraphicsPipelineStateLocalCache> CGraphicsPipelineStateLocalCachePtr;
